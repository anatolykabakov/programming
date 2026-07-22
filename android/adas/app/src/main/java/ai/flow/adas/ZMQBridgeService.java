package ai.flow.adas;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import org.zeromq.ZContext;
import org.zeromq.ZMQ;
import org.zeromq.ZMQ.Poller;
import org.zeromq.ZMQ.Socket;

import java.nio.charset.StandardCharsets;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

import ai.flow.adas.Messages.ZMQMessage;

/**
 * Java side of the native ZmqBridgeService.
 *
 * Endpoints from {@code assets/config.json} → {@code zmq.endpoint_in/out}
 * (defaults tcp://127.0.0.1:5555 / :5556).
 *
 * Multipart: [topic UTF-8][ZMQMessage protobuf].
 *
 * JeroMQ sockets are not thread-safe: all pubIn sends run on {@link #zmqExecutor}
 * via {@link #inboundQueue}; subOut is only touched in {@link #messageLoop}.
 */
public class ZMQBridgeService extends Service {
    private static final String TAG = "ZMQBridgeService";

    private static final String DEFAULT_ENDPOINT_IN = "tcp://127.0.0.1:5555";
    private static final String DEFAULT_ENDPOINT_OUT = "tcp://127.0.0.1:5556";

    private static final AtomicReference<ZMQBridgeService> INSTANCE = new AtomicReference<>();
    private static final AtomicReference<OutboundListener> OUTBOUND_LISTENER = new AtomicReference<>();

    private static final class InboundFrame {
        final byte[] topic;
        final byte[] body;

        InboundFrame(byte[] topic, byte[] body) {
            this.topic = topic;
            this.body = body;
        }
    }

    /** Live UI / debug sink for native → Java outbound messages (lane_keep, steer, …). */
    public interface OutboundListener {
        void onOutbound(String topic, ZMQMessage message);
    }

    public static void setOutboundListener(OutboundListener listener) {
        OUTBOUND_LISTENER.set(listener);
    }

    private ZContext context;
    private Socket pubIn;
    private Socket subOut;
    private Poller poller;
    private ExecutorService zmqExecutor;
    private final ConcurrentLinkedQueue<InboundFrame> inboundQueue = new ConcurrentLinkedQueue<>();
    private volatile boolean isRunning = false;
    private String endpointIn = DEFAULT_ENDPOINT_IN;
    private String endpointOut = DEFAULT_ENDPOINT_OUT;

    public static ZMQBridgeService getInstance() {
        return INSTANCE.get();
    }

    @Override
    public void onCreate() {
        super.onCreate();
        INSTANCE.set(this);
        Log.i(TAG, "ZMQBridgeService created");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "ZMQBridgeService starting...");
        startZMQBridge();
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "ZMQBridgeService destroying...");
        stopZMQBridge();
        INSTANCE.compareAndSet(this, null);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private void startZMQBridge() {
        try {
            setupZMQ();
            zmqExecutor = Executors.newSingleThreadExecutor();
            zmqExecutor.submit(this::messageLoop);
            isRunning = true;
            Log.i(TAG, "ZMQ Bridge started (PUB→" + endpointIn + ", SUB←" + endpointOut + ")");
        } catch (Exception e) {
            Log.e(TAG, "Error starting ZMQ Bridge", e);
        }
    }

    private void stopZMQBridge() {
        Log.i(TAG, "Stopping ZMQ Bridge...");
        isRunning = false;

        if (zmqExecutor != null) {
            zmqExecutor.shutdown();
            try {
                if (!zmqExecutor.awaitTermination(5, TimeUnit.SECONDS)) {
                    zmqExecutor.shutdownNow();
                }
            } catch (InterruptedException e) {
                zmqExecutor.shutdownNow();
                Thread.currentThread().interrupt();
            }
            zmqExecutor = null;
        }

        if (context != null) {
            context.close();
            context = null;
        }
        pubIn = null;
        subOut = null;
        poller = null;

        Log.i(TAG, "ZMQ Bridge stopped");
    }

    private void setupZMQ() {
        AdasConfig cfg = AdasConfig.load(this);
        endpointIn = (cfg.zmqEndpointIn != null && !cfg.zmqEndpointIn.isEmpty())
                ? cfg.zmqEndpointIn : DEFAULT_ENDPOINT_IN;
        endpointOut = (cfg.zmqEndpointOut != null && !cfg.zmqEndpointOut.isEmpty())
                ? cfg.zmqEndpointOut : DEFAULT_ENDPOINT_OUT;

        context = new ZContext();

        // Sensors / inject → native (native binds SUB)
        pubIn = context.createSocket(ZMQ.PUB);
        pubIn.connect(endpointIn);
        pubIn.setSendTimeOut(10);
        Log.i(TAG, "ZMQ PUB connected to " + endpointIn);

        // Native → bag (native binds PUB)
        subOut = context.createSocket(ZMQ.SUB);
        subOut.connect(endpointOut);
        subOut.subscribe("".getBytes(StandardCharsets.UTF_8));
        subOut.setReceiveTimeOut(10);
        Log.i(TAG, "ZMQ SUB connected to " + endpointOut);

        poller = context.createPoller(1);
        poller.register(subOut, Poller.POLLIN);
    }

    private void messageLoop() {
        Log.d(TAG, "ZMQ message loop started");
        while (isRunning) {
            try {
                flushInboundQueue();
                if (poller == null) {
                    Thread.sleep(10);
                    continue;
                }
                int events = poller.poll(10);
                if (events > 0 && poller.pollin(0)) {
                    // Drain outbound burst so bag/UI keep up with can/rx.
                    for (int i = 0; i < 64; i++) {
                        if (!recvOutbound()) {
                            break;
                        }
                    }
                }
            } catch (Exception e) {
                Log.e(TAG, "Error in ZMQ message loop", e);
            }
        }
        flushInboundQueue();
        Log.d(TAG, "ZMQ message loop stopped");
    }

    /** Send all queued Java→native frames on the ZMQ thread only. */
    private void flushInboundQueue() {
        Socket pub = pubIn;
        if (pub == null) {
            inboundQueue.clear();
            return;
        }
        InboundFrame frame;
        while ((frame = inboundQueue.poll()) != null) {
            try {
                boolean ok = pub.sendMore(frame.topic) && pub.send(frame.body, ZMQ.DONTWAIT);
                if (!ok) {
                    Log.d(TAG, "ZMQ inbound send dropped");
                }
            } catch (Exception e) {
                Log.e(TAG, "Error publishing inbound frame", e);
            }
        }
    }

    /** @return false if no message was available */
    private boolean recvOutbound() {
        try {
            // multipart [topic][payload], or single-frame protobuf
            byte[] first = subOut.recv(ZMQ.DONTWAIT);
            if (first == null || first.length == 0) {
                return false;
            }

            String topic;
            byte[] payload;
            if (subOut.hasReceiveMore()) {
                topic = new String(first, StandardCharsets.UTF_8);
                payload = subOut.recv(ZMQ.DONTWAIT);
                if (payload == null) {
                    Log.w(TAG, "Outbound multipart missing payload for " + topic);
                    return true;
                }
            } else {
                payload = first;
                topic = null;
            }

            ZMQMessage zmqMsg = ZMQMessage.parseFrom(payload);
            if (topic == null || topic.isEmpty()) {
                topic = zmqMsg.getTopic();
            }
            Log.d(TAG, "ZMQ outbound '" + topic + "' " + payload.length + " bytes");
            Logger.getInstance().logZMQMessage(zmqMsg);
            OutboundListener listener = OUTBOUND_LISTENER.get();
            if (listener != null) {
                try {
                    listener.onOutbound(topic, zmqMsg);
                } catch (Exception cbEx) {
                    Log.e(TAG, "Outbound listener error for '" + topic + "'", cbEx);
                }
            }
            return true;
        } catch (Exception e) {
            Log.e(TAG, "Error receiving outbound ZMQ", e);
            return true;
        }
    }

    /**
     * Queue a message for native (ZMQ IN). Safe from any thread.
     * Prefer {@link #publishToNative(ZMQMessage)} at call sites.
     */
    public void publishToNative(String topic, ZMQMessage message) {
        if (!isRunning || pubIn == null) {
            Log.w(TAG, "ZMQ Bridge not running");
            return;
        }
        if (message == null) {
            return;
        }
        try {
            ZMQMessage.Builder b = message.toBuilder();
            if (b.getTopic().isEmpty() && topic != null) {
                b.setTopic(topic);
            }
            byte[] body = b.build().toByteArray();
            String t = (topic != null && !topic.isEmpty()) ? topic : b.getTopic();
            if (t == null || t.isEmpty()) {
                Log.w(TAG, "Dropping inbound with empty topic");
                return;
            }
            inboundQueue.offer(new InboundFrame(t.getBytes(StandardCharsets.UTF_8), body));
        } catch (Exception e) {
            Log.e(TAG, "Error queueing publish to native for topic: " + topic, e);
        }
    }

    /** Publish into native over ZMQ IN. No-op if the bridge service is not running. */
    public static void publishToNative(ZMQMessage message) {
        if (message == null) {
            return;
        }
        ZMQBridgeService bridge = getInstance();
        if (bridge == null || !bridge.isRunning()) {
            return;
        }
        bridge.publishToNative(message.getTopic(), message);
    }

    public boolean isRunning() {
        return isRunning;
    }
}
