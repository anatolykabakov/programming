package ai.flow.adas;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;
import org.zeromq.ZMQ;
import org.zeromq.ZContext;
import org.zeromq.ZMQ.Socket;
import org.zeromq.ZMQ.Poller;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.TimeUnit;

import ai.flow.adas.Messages.ZMQMessage;
import ai.flow.adas.Logger;

public class ZMQBridgeService extends Service {
    private static final String TAG = "ZMQBridgeService";

    // controls/steer (:5564) is owned by an external PUB (test_steer_tx.py);
    // native ZmqBridgeService SUBs connect there. Do not bind that port here.
    private static final Map<String, String> DEFAULT_PUB_TOPICS = new HashMap<String, String>() {{
        put("sensors/imu", "tcp://127.0.0.1:5558");            // IMU data
        put("sensors/gps/location", "tcp://127.0.0.1:5557");   // GPS location
        put("sensors/camera/image", "tcp://127.0.0.1:5556");   // Camera image
    }};

    private static final Map<String, String> DEFAULT_SUB_TOPICS = new HashMap<String, String>() {{
        put("can/rx", "tcp://127.0.0.1:5563");
        put("panda/health", "tcp://127.0.0.1:5565");
        put("vehicle/state", "tcp://127.0.0.1:5566");
    }};

    private ZContext context;
    private Poller poller;
    private Map<String, Socket> topicSubscribers;
    private Map<String, Socket> topicPublishers;
    private ExecutorService zmqExecutor;
    private boolean isRunning = false;

    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "ZMQBridgeService created");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "ZMQBridgeService starting...");
        startZMQBridge();
        return START_STICKY; // Restart service if killed
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "ZMQBridgeService destroying...");
        stopZMQBridge();
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
            Log.i(TAG, "ZMQ Bridge started successfully");
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
        }

        if (context != null) {
            context.close();
        }

        Log.i(TAG, "ZMQ Bridge stopped");
    }

    private void setupZMQ() {
        Log.d(TAG, "Setting up ZMQ...");

        try {
            context = new ZContext();
            topicSubscribers = new HashMap<>();
            topicPublishers = new HashMap<>();

            for (Map.Entry<String, String> entry : DEFAULT_SUB_TOPICS.entrySet()) {
                String topic = entry.getKey();
                String endpoint = entry.getValue();

                Socket subscriber = context.createSocket(ZMQ.SUB);
                subscriber.connect(endpoint);
                subscriber.subscribe("".getBytes()); // Subscribe to all messages
                subscriber.setReceiveTimeOut(10); // 10ms timeout (non-blocking)

                topicSubscribers.put(topic, subscriber);
                Log.d(TAG, "ZMQ SUB socket for topic '" + topic + "' CONNECTED to " + endpoint);
            }

            for (Map.Entry<String, String> entry : DEFAULT_PUB_TOPICS.entrySet()) {
                String topic = entry.getKey();
                String endpoint = entry.getValue();

                Socket publisher = context.createSocket(ZMQ.PUB);
                publisher.bind(endpoint);
                topicPublishers.put(topic, publisher);
                Log.d(TAG, "ZMQ PUB socket for topic '" + topic + "' BOUND to " + endpoint);
            }

            if (!topicSubscribers.isEmpty()) {
                poller = context.createPoller(topicSubscribers.size());
                for (Map.Entry<String, Socket> entry : topicSubscribers.entrySet()) {
                    poller.register(entry.getValue(), Poller.POLLIN);
                }
            }

            Log.d(TAG, "ZMQ setup completed successfully");
        } catch (Exception e) {
            Log.e(TAG, "Exception in setupZMQ(): " + e.getMessage(), e);
            throw new RuntimeException("Failed to setup ZMQ", e);
        }
    }

    private void messageLoop() {
        Log.d(TAG, "ZMQ Message loop started");

        while (isRunning) {
            try {
                if (poller != null) {
                    int events = poller.poll(10); // 10ms timeout
                    if (events > 0) {
                        for (int i = 0; i < poller.getSize(); i++) {
                            if (poller.pollin(i)) {
                                Socket socket = topicSubscribers.get(getTopicByIndex(i));
                                if (socket != null) {
                                    byte[] message = socket.recv(ZMQ.DONTWAIT);
                                    if (message != null && message.length > 0) {
                                        processExternalMessage(getTopicByIndex(i), message);
                                    }
                                }
                            }
                        }
                    }
                }
            } catch (Exception e) {
                Log.e(TAG, "Error in ZMQ message loop", e);
            }
        }

        Log.d(TAG, "ZMQ Message loop stopped");
    }

    private void processExternalMessage(String topic, byte[] message) {
        try {
            Log.d(TAG, "Processing external ZMQ message for topic: " + topic + ", size: " + message.length + " bytes");

            ZMQMessage zmqMsg = ZMQMessage.parseFrom(message);

            Logger.getInstance().logZMQMessage(zmqMsg);

            // if (zmqMsg.hasCanData()) {
            //     processCANRxMessage(zmqMsg.getCanData());
            // } else if (zmqMsg.hasPandaHealth()) {
            //     processPandaHealthMessage(zmqMsg.getPandaHealth());
            // } else {
            //     Log.d(TAG, "Received message for topic: " + topic + " (no specific processing)");
            // }

        } catch (Exception e) {
            Log.e(TAG, "Error processing external message for topic: " + topic, e);
        }
    }

    private void processCANRxMessage(ai.flow.adas.Can.CANData canData) {
        try {
            for (int i = 0; i < canData.getFramesCount(); i++) {
                ai.flow.adas.Can.CANFrame frame = canData.getFrames(i);

                String hexData = "";
                if (frame.getData() != null && frame.getData().size() > 0) {
                    StringBuilder sb = new StringBuilder();
                    for (int j = 0; j < frame.getData().size(); j++) {
                        if (j > 0) sb.append(" ");
                        sb.append(String.format("%02X", frame.getData().byteAt(j)));
                    }
                    hexData = sb.toString();
                }

                String logEntry = String.format("%d,%d,%d,%d,%s",
                    canData.getTimestamp(),
                    frame.getAddress(),
                    frame.getBusTime(),
                    frame.getData().size(),
                    hexData);
                Logger.getInstance().logCAN(logEntry);
            }

            Log.d(TAG, "Processed CAN RX message with " + canData.getFramesCount() + " frames");
        } catch (Exception e) {
            Log.e(TAG, "Error processing CAN RX message", e);
        }
    }

    private void processPandaHealthMessage(ai.flow.adas.Panda.PandaHealth zmqMsg) {
        try {
            String logEntry = String.format("%d, %s, %d, %d, %d",
                zmqMsg.getTimestamp(),
                zmqMsg.getControlsAllowed(),
                zmqMsg.getVoltageMv(),
                zmqMsg.getCurrentMa(),
                zmqMsg.getSafetyMode());

            Logger.getInstance().logPandaHealth(logEntry);
            Log.d(TAG, "Processed panda health message: " + logEntry);
        } catch (Exception e) {
            Log.e(TAG, "Error processing panda health message", e);
        }
    }

    private String getTopicByIndex(int index) {
        int i = 0;
        for (String topic : topicSubscribers.keySet()) {
            if (i == index) {
                return topic;
            }
            i++;
        }
        return null;
    }

    /**
     * Publish internal message to external ZMQ
     */
    public void publishInternalMessage(String topic, ZMQMessage message) {
        if (!isRunning || topicPublishers == null) {
            Log.w(TAG, "ZMQ Bridge not running or publishers not initialized");
            return;
        }

        Socket publisher = topicPublishers.get(topic);
        if (publisher == null) {
            Log.w(TAG, "No publisher found for topic: " + topic);
            return;
        }

        try {
            byte[] serializedData = message.toByteArray();
            boolean result = publisher.send(serializedData, ZMQ.DONTWAIT);

            if (result) {
                Log.d(TAG, "Sent internal message to external ZMQ for topic '" + topic + "' SUCCESS");
            } else {
                Log.e(TAG, "Failed to send message to external ZMQ for topic '" + topic + "'");
            }
        } catch (Exception e) {
            Log.e(TAG, "Error publishing internal message for topic: " + topic, e);
        }
    }

    public boolean isRunning() {
        return isRunning;
    }
}
