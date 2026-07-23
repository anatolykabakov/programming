package ai.flow.adas;

import android.util.Log;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Map;
import java.util.List;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.ConcurrentHashMap;
import bag.BagOuterClass;

public class BagLogger {
    private static final String TAG = "BagLogger";

    private static final long MAX_SIZE_BYTES = 50 * 1024 * 1024;

    private static volatile BagLogger instance;
    private static final Object lock = new Object();

    private final AtomicBoolean running = new AtomicBoolean(false);
    private File baseDirectory;

    private final Map<String, List<Messages.ZMQMessage>> topicBuffers = new ConcurrentHashMap<>();
    private final Map<String, Integer> bufferSizes = new ConcurrentHashMap<>();

    private final Map<String, Long> topicFileSizes = new ConcurrentHashMap<>();

    private java.util.Timer flushTimer;

    private BagLogger() {
    }

    public static BagLogger getInstance() {
        if (instance == null) {
            synchronized (lock) {
                if (instance == null) {
                    instance = new BagLogger();
                }
            }
        }
        return instance;
    }

    public void startInDirectory(File dir) {
        synchronized (this) {
            if (running.get()) {
                Log.i(TAG, "BagLogger already running");
                return;
            }
            if (dir == null) {
                Log.e(TAG, "BagLogger startInDirectory: dir is null");
                return;
            }
            if (!dir.exists() && !dir.mkdirs()) {
                Log.e(TAG, "Failed to create bag directory: " + dir.getAbsolutePath());
                return;
            }

            baseDirectory = dir;
            running.set(true);
            startPeriodicFlushLocked();
            Log.i(TAG, "BagLogger started in " + dir.getAbsolutePath());
        }
    }

    public void stop() {
        synchronized (this) {
            if (!running.get()) {
                return;
            }

            Log.i(TAG, "Stopping BagLogger...");
            running.set(false);

            if (flushTimer != null) {
                flushTimer.cancel();
                flushTimer = null;
            }

            flushAllBuffers();

            topicBuffers.clear();
            bufferSizes.clear();
            topicFileSizes.clear();

            Log.i(TAG, "BagLogger stopped. Bag directory preserved: "
                    + (baseDirectory != null ? baseDirectory.getAbsolutePath() : "null"));
        }
    }

    public void addMessage(String topic, Messages.ZMQMessage message) {
        if (!running.get() || !ProtoUtils.isValidForBagLogging(message)) {
            return;
        }

        String fixedTopicName = ProtoUtils.fixTopicName(topic);

        synchronized (this) {
            if (!running.get()) {
                return;
            }

            List<Messages.ZMQMessage> buffer = topicBuffers.get(fixedTopicName);
            if (buffer == null) {
                buffer = new ArrayList<>();
                topicBuffers.put(fixedTopicName, buffer);
                bufferSizes.put(fixedTopicName, 0);
            }

            buffer.add(message);
            int messageSize = ProtoUtils.getMessageSize(message);
            bufferSizes.put(fixedTopicName, bufferSizes.get(fixedTopicName) + messageSize);

            if (bufferSizes.get(fixedTopicName) >= MAX_SIZE_BYTES) {
                Log.d(TAG, "Buffer full for topic " + fixedTopicName
                        + " (messages: " + buffer.size()
                        + ", size: " + bufferSizes.get(fixedTopicName) + " bytes), flushing...");
                flushTopicBuffer(fixedTopicName);
            }
        }
    }

    private void startPeriodicFlushLocked() {
        if (flushTimer != null) {
            flushTimer.cancel();
        }
        flushTimer = new java.util.Timer("BagFlush", true);
        flushTimer.scheduleAtFixedRate(new java.util.TimerTask() {
            @Override
            public void run() {
                synchronized (BagLogger.this) {
                    if (!running.get()) {
                        return;
                    }
                    flushAllBuffers();
                }
            }
        }, 10_000, 10_000);
    }

    private void flushTopicBuffer(String fixedTopicName) {
        List<Messages.ZMQMessage> buffer = topicBuffers.get(fixedTopicName);
        if (buffer == null || buffer.isEmpty()) {
            return;
        }

        try {
            File topicDir = new File(baseDirectory, fixedTopicName);
            if (!topicDir.exists()) {
                boolean created = topicDir.mkdirs();
                Log.d(TAG, "Created topic directory: " + topicDir.getAbsolutePath() + ", success: " + created);
            }

            BagOuterClass.Bag bag = ProtoUtils.createBagMessage(new ArrayList<>(buffer));

            String fileName = getNextFileName(fixedTopicName);
            File bagFile = new File(topicDir, fileName);

            try (FileOutputStream fos = new FileOutputStream(bagFile)) {
                bag.writeTo(fos);
                fos.flush();

                long fileSize = bagFile.length();

                topicFileSizes.put(fixedTopicName,
                    topicFileSizes.getOrDefault(fixedTopicName, 0L) + fileSize);

                Log.d(TAG, "Written bag file: " + bagFile.getAbsolutePath() +
                          " (messages: " + buffer.size() +
                          ", size: " + fileSize + " bytes" +
                          ", total topic size: " + topicFileSizes.get(fixedTopicName) + " bytes)");
            }

            buffer.clear();
            bufferSizes.put(fixedTopicName, 0);

        } catch (IOException e) {
            Log.e(TAG, "Error writing bag file for topic: " + fixedTopicName, e);
        }
    }

    private void flushAllBuffers() {
        Log.i(TAG, "Flushing all buffers...");

        for (String fixedTopicName : new ArrayList<>(topicBuffers.keySet())) {
            flushTopicBuffer(fixedTopicName);
        }
    }

    private String getNextFileName(String fixedTopicName) {
        long currentSize = topicFileSizes.getOrDefault(fixedTopicName, 0L);

        if (currentSize >= MAX_SIZE_BYTES) {
            topicFileSizes.put(fixedTopicName, 0L);

            Log.i(TAG, "Creating new bag file for topic " + fixedTopicName +
                      " (previous size: " + currentSize + " bytes)");
        }

        return ProtoUtils.createDataFileName(System.currentTimeMillis());
    }
}
