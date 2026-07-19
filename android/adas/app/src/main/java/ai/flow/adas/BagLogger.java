package ai.flow.adas;

import android.util.Log;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.Map;
import java.util.HashMap;
import java.util.List;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.ConcurrentHashMap;
import bag.BagOuterClass;

/**
 * Логгер для записи protobuf сообщений в bag файлы
 * Структура: BAG_YYYY_MM_DD_HH_mm_ss.tar.gz / __topic__name/YYYY_MM_DD_HH_mm_ss.bin
 */
public class BagLogger {
    private static final String TAG = "BagLogger";

    // Максимальный размер буфера/файла (50MB)
    private static final long MAX_SIZE_BYTES = 50 * 1024 * 1024;

    private static volatile BagLogger instance;
    private static final Object lock = new Object();

    private AtomicBoolean running = new AtomicBoolean(false);
    private File baseDirectory;
    private String currentBagName;

    // Буферы сообщений по топикам
    private Map<String, List<Messages.ZMQMessage>> topicBuffers = new ConcurrentHashMap<>();
    private Map<String, Integer> bufferSizes = new ConcurrentHashMap<>();

    // Отслеживание размера файлов по топикам
    private Map<String, Long> topicFileSizes = new ConcurrentHashMap<>();

    // Таймеры для периодической записи
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

    public void start(String basePath) {
        if (running.get()) {
            Log.i(TAG, "BagLogger already running");
            return;
        }

        Log.i(TAG, "Starting BagLogger...");

        String ts = new SimpleDateFormat("yyyy_MM_dd_HH_mm_ss", Locale.US).format(new Date());
        File dir = new File(basePath, ts);
        if (!dir.exists()) {
            boolean created = dir.mkdirs();
            if (!created) {
                Log.e(TAG, "Failed to create bag directory: " + dir.getAbsolutePath());
                return;
            }
            Log.i(TAG, "Created new bag directory: " + dir.getAbsolutePath());
        } else {
            Log.i(TAG, "Using existing bag directory: " + dir.getAbsolutePath());
        }

        startInDirectory(dir);
    }

    /** Use an existing session directory (shared with text Logger). */
    public void startInDirectory(File dir) {
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
        currentBagName = "BAG_" + dir.getName();
        running.set(true);
        startPeriodicFlush();
        Log.i(TAG, "BagLogger started in " + dir.getAbsolutePath());
    }

    public void stop() {
        if (!running.get()) return;

        Log.i(TAG, "Stopping BagLogger...");
        running.set(false);

        if (flushTimer != null) {
            flushTimer.cancel();
            flushTimer = null;
        }

        // Записываем все оставшиеся сообщения
        flushAllBuffers();

        // Очищаем буферы
        topicBuffers.clear();
        bufferSizes.clear();
        topicFileSizes.clear();

        Log.i(TAG, "BagLogger stopped. Bag directory preserved: " + (baseDirectory != null ? baseDirectory.getAbsolutePath() : "null"));
    }

    /**
     * Добавляет сообщение в буфер для указанного топика
     */
    public void addMessage(String topic, Messages.ZMQMessage message) {
        Log.d(TAG, "BagLogger.addMessage called for topic: " + topic + ", running: " + running.get() + ", valid: " + ProtoUtils.isValidForBagLogging(message));

        if (!running.get() || !ProtoUtils.isValidForBagLogging(message)) {
            Log.d(TAG, "Skipping message - running: " + running.get() + ", valid: " + ProtoUtils.isValidForBagLogging(message));
            return;
        }

        String fixedTopicName = ProtoUtils.fixTopicName(topic);

        synchronized (this) {
            // Получаем или создаем буфер для топика
            List<Messages.ZMQMessage> buffer = topicBuffers.get(fixedTopicName);
            if (buffer == null) {
                buffer = new ArrayList<>();
                topicBuffers.put(fixedTopicName, buffer);
                bufferSizes.put(fixedTopicName, 0);

                // Запускаем таймер для периодической записи
                // startTopicTimer(fixedTopicName);
            }

            // Добавляем сообщение
            buffer.add(message);
            int messageSize = ProtoUtils.getMessageSize(message);
            bufferSizes.put(fixedTopicName, bufferSizes.get(fixedTopicName) + messageSize);

            // Проверяем, нужно ли записать буфер (50MB)
            if (bufferSizes.get(fixedTopicName) >= MAX_SIZE_BYTES) {

                Log.d(TAG, "Buffer full for topic " + fixedTopicName +
                          " (messages: " + buffer.size() +
                          ", size: " + bufferSizes.get(fixedTopicName) + " bytes), flushing...");

                flushTopicBuffer(fixedTopicName);
            }
        }
    }

    private void startPeriodicFlush() {
        if (flushTimer != null) {
            flushTimer.cancel();
        }
        flushTimer = new java.util.Timer("BagFlush", true);
        flushTimer.scheduleAtFixedRate(new java.util.TimerTask() {
            @Override
            public void run() {
                if (!running.get()) {
                    return;
                }
                synchronized (BagLogger.this) {
                    flushAllBuffers();
                }
            }
        }, 10_000, 10_000);
    }
    /**
     * Записывает буфер топика в файл
     */
    private void flushTopicBuffer(String fixedTopicName) {
        List<Messages.ZMQMessage> buffer = topicBuffers.get(fixedTopicName);
        if (buffer == null || buffer.isEmpty()) {
            return;
        }

        try {
            // Создаем директорию для топика
            File topicDir = new File(baseDirectory, fixedTopicName);
            if (!topicDir.exists()) {
                boolean created = topicDir.mkdirs();
                Log.d(TAG, "Created topic directory: " + topicDir.getAbsolutePath() + ", success: " + created);
            }

            // Создаем bag сообщение
            BagOuterClass.Bag bag = ProtoUtils.createBagMessage(new ArrayList<>(buffer));

            // Проверяем размер текущего файла и создаем новый если нужно
            String fileName = getNextFileName(fixedTopicName);
            File bagFile = new File(topicDir, fileName);

            // Записываем в файл
            try (FileOutputStream fos = new FileOutputStream(bagFile)) {
                bag.writeTo(fos);
                fos.flush();

                long fileSize = bagFile.length();

                // Обновляем размер файла для топика
                topicFileSizes.put(fixedTopicName,
                    topicFileSizes.getOrDefault(fixedTopicName, 0L) + fileSize);

                Log.d(TAG, "Written bag file: " + bagFile.getAbsolutePath() +
                          " (messages: " + buffer.size() +
                          ", size: " + fileSize + " bytes" +
                          ", total topic size: " + topicFileSizes.get(fixedTopicName) + " bytes)");
            }

            // Очищаем буфер
            buffer.clear();
            bufferSizes.put(fixedTopicName, 0);

        } catch (IOException e) {
            Log.e(TAG, "Error writing bag file for topic: " + fixedTopicName, e);
        }
    }

    /**
     * Записывает все буферы в файлы
     */
    private void flushAllBuffers() {
        Log.i(TAG, "Flushing all buffers...");

        for (String fixedTopicName : topicBuffers.keySet()) {
            flushTopicBuffer(fixedTopicName);
        }
    }

    /**
     * Принудительно записывает буфер указанного топика
     */
    public void flushTopic(String topic) {
        if (!running.get()) return;

        String fixedTopicName = ProtoUtils.fixTopicName(topic);
        synchronized (this) {
            flushTopicBuffer(fixedTopicName);
        }
    }

    public boolean isRunning() {
        return running.get();
    }

    /**
     * Получает имя следующего файла для топика
     */
    private String getNextFileName(String fixedTopicName) {
        long currentSize = topicFileSizes.getOrDefault(fixedTopicName, 0L);

        // Если размер превышает лимит, создаем новый файл
        if (currentSize >= MAX_SIZE_BYTES) {
            topicFileSizes.put(fixedTopicName, 0L); // Сбрасываем счетчик размера

            Log.i(TAG, "Creating new bag file for topic " + fixedTopicName +
                      " (previous size: " + currentSize + " bytes)");

            // Создаем новый файл с текущим временем
            return ProtoUtils.createDataFileName(System.currentTimeMillis());
        }

        return ProtoUtils.createDataFileName(System.currentTimeMillis());
    }

    /**
     * Получает статистику по буферам
     */
    public String getBufferStats() {
        if (!running.get()) return "BagLogger not running";

        StringBuilder stats = new StringBuilder("BagLogger Stats:\n");
        synchronized (this) {
            for (String topic : topicBuffers.keySet()) {
                List<Messages.ZMQMessage> buffer = topicBuffers.get(topic);
                int size = bufferSizes.get(topic);
                long fileSize = topicFileSizes.getOrDefault(topic, 0L);
                int fileCount = 1; // Всегда 1, так как используем время в имени файла
                stats.append("  ").append(topic).append(": ")
                     .append(buffer.size()).append(" messages in buffer, ")
                     .append(size).append(" bytes buffer, ")
                     .append(fileSize).append(" bytes total, ")
                     .append(fileCount).append(" files\n");
            }
        }
        return stats.toString();
    }
}
