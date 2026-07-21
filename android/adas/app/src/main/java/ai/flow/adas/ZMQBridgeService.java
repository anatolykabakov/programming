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
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

import ai.flow.adas.Messages.ZMQMessage;

/**
 * Java side of the native ZmqBridgeService.
 *
 * Two endpoints only:
 *   IN  tcp://127.0.0.1:5555 — connect PUB  (sensors / commands → native SUB)
 *   OUT tcp://127.0.0.1:5556 — connect SUB  (native PUB → bag logger)
 *
 * Multipart: [topic UTF-8][ZMQMessage protobuf].
 */
public class ZMQBridgeService extends Service {
    private static final String TAG = "ZMQBridgeService";

    public static final String ENDPOINT_IN = "tcp://127.0.0.1:5555";
    public static final String ENDPOINT_OUT = "tcp://127.0.0.1:5556";

    private static final AtomicReference<ZMQBridgeService> INSTANCE = new AtomicReference<>();
    private static final AtomicReference<OutboundListener> OUTBOUND_LISTENER = new AtomicReference<>();

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
    private volatile boolean isRunning = false;

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
            Log.i(TAG, "ZMQ Bridge started (PUB→" + ENDPOINT_IN + ", SUB←" + ENDPOINT_OUT + ")");
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
        context = new ZContext();

        // Sensors / inject → native (native binds SUB)
        pubIn = context.createSocket(ZMQ.PUB);
        pubIn.connect(ENDPOINT_IN);
        pubIn.setSendTimeOut(10);
        Log.i(TAG, "ZMQ PUB connected to " + ENDPOINT_IN);

        // Native → bag (native binds PUB)
        subOut = context.createSocket(ZMQ.SUB);
        subOut.connect(ENDPOINT_OUT);
        subOut.subscribe("".getBytes(StandardCharsets.UTF_8));
        subOut.setReceiveTimeOut(10);
        Log.i(TAG, "ZMQ SUB connected to " + ENDPOINT_OUT);

        poller = context.createPoller(1);
        poller.register(subOut, Poller.POLLIN);
    }

    private void messageLoop() {
        Log.d(TAG, "ZMQ message loop started");
        while (isRunning) {
            try {
                if (poller == null) {
                    Thread.sleep(10);
                    continue;
                }
                int events = poller.poll(10);
                if (events > 0 && poller.pollin(0)) {
                    recvOutbound();
                }
            } catch (Exception e) {
                Log.e(TAG, "Error in ZMQ message loop", e);
            }
        }
        Log.d(TAG, "ZMQ message loop stopped");
    }

    private void recvOutbound() {
        try {
            // multipart [topic][payload], or single-frame protobuf
            byte[] first = subOut.recv(ZMQ.DONTWAIT);
            if (first == null || first.length == 0) {
                return;
            }

            String topic;
            byte[] payload;
            if (subOut.hasReceiveMore()) {
                topic = new String(first, StandardCharsets.UTF_8);
                payload = subOut.recv(ZMQ.DONTWAIT);
                if (payload == null) {
                    Log.w(TAG, "Outbound multipart missing payload for " + topic);
                    return;
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
        } catch (Exception e) {
            Log.e(TAG, "Error receiving outbound ZMQ", e);
        }
    }

    /**
     * Publish a sensor / command message to native (IN endpoint).
     */
    public void publishInternalMessage(String topic, ZMQMessage message) {
        if (!isRunning || pubIn == null) {
            Log.w(TAG, "ZMQ Bridge not running");
            return;
        }
        try {
            ZMQMessage.Builder b = message.toBuilder();
            if (b.getTopic().isEmpty() && topic != null) {
                b.setTopic(topic);
            }
            byte[] body = b.build().toByteArray();
            String t = (topic != null && !topic.isEmpty()) ? topic : b.getTopic();
            boolean ok = pubIn.sendMore(t.getBytes(StandardCharsets.UTF_8))
                    && pubIn.send(body, ZMQ.DONTWAIT);
            if (!ok) {
                Log.d(TAG, "ZMQ inbound send dropped for '" + t + "'");
            }
        } catch (Exception e) {
            Log.e(TAG, "Error publishing to native for topic: " + topic, e);
        }
    }

    public boolean isRunning() {
        return isRunning;
    }
}
