package ai.flow.android;

import android.util.Log;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Rect;
import android.graphics.YuvImage;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ByteArrayOutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.Map;
import java.util.HashMap;
import java.util.concurrent.atomic.AtomicBoolean;

public class Logger {
    private static final String TAG = "Logger";

    private static volatile Logger instance;
    private static final Object lock = new Object();

    private AtomicBoolean running = new AtomicBoolean(false);
    private File logDirectory;
    private Map<String, FileOutputStream> logFiles = new HashMap<>();
    
    // Bag logger для protobuf сообщений
    private BagLogger bagLogger;

    private Logger() {
    }

    public static Logger getInstance() {
        if (instance == null) {
            synchronized (lock) {
                if (instance == null) {
                    instance = new Logger();
                }
            }
        }
        return instance;
    }

    public void start(String basePath) {
        if (running.get()) {
            Log.i(TAG, "Logger already running");
            return;
        }

        Log.i(TAG, "Starting Logger...");

        String ts = new SimpleDateFormat("yyyy_MM_dd_HH_mm_ss", Locale.US).format(new Date());
        File dir = new File(basePath, ts);
        if (!dir.exists()) {
            boolean created = dir.mkdirs();
            if (!created) {
                Log.e(TAG, "Failed to create log directory: " + dir.getAbsolutePath());
                return;
            }
            Log.i(TAG, "Created new log directory: " + dir.getAbsolutePath());
        } else {
            Log.i(TAG, "Using existing log directory: " + dir.getAbsolutePath());
        }

        logDirectory = dir;
        running.set(true);
        
        // Инициализируем bag logger
        bagLogger = BagLogger.getInstance();
        bagLogger.start(basePath);
        
        Log.i(TAG, "Logger started successfully");
    }

    public void stop() {
        if (!running.get()) return;

        running.set(false);

        for (FileOutputStream fos : logFiles.values()) {
            try {
                fos.close();
            } catch (IOException e) {
                Log.e(TAG, "Error closing log file", e);
            }
        }
        logFiles.clear();
        
        // Останавливаем bag logger
        if (bagLogger != null) {
            bagLogger.stop();
            bagLogger = null;
        }

        Log.i(TAG, "Logger stopped. Log directory preserved: " + (logDirectory != null ? logDirectory.getAbsolutePath() : "null"));
    }


    private void logToFile(String filename, String data) {
        try {
            FileOutputStream fos = logFiles.get(filename);
            if (fos == null) {
                File file = new File(logDirectory, filename + ".txt");
                fos = new FileOutputStream(file, true);
                logFiles.put(filename, fos);
                Log.i(TAG, "Creating new log file: " + file.getAbsolutePath());
            }

            fos.write((data + "\n").getBytes());
            fos.flush();
            Log.d(TAG, "Successfully wrote to " + filename + ".txt: " + data);
        } catch (IOException e) {
            Log.e(TAG, "Error writing to " + filename + ".txt", e);
        }
    }

    public void logGPS(long timestamp, double latitude, double longitude, double altitude, float speed, float bearing, float accuracy) {
        if (!running.get()) return;
        String logEntry = String.format(Locale.US, "%d, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f",
            timestamp, latitude, longitude, altitude, speed, bearing, accuracy);
        logToFile("gps", logEntry);
    }

    public void logIMU(long timestamp, float ax, float ay, float az, float gx, float gy, float gz,
                       float mx, float my, float mz) {
        if (!running.get()) return;
        String logEntry = String.format(Locale.US, "%d, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f",
            timestamp, ax, ay, az, gx, gy, gz, mx, my, mz);
        logToFile("imu", logEntry);
    }

    public void logCAN(String data) {
        if (!running.get()) return;
        logToFile("can_frames", data);
    }

    public void logPandaHealth(String data) {
        if (!running.get()) return;
        logToFile("panda_health", data);
    }

    public void logCameraImage(long timestamp, Bitmap bitmap) {
        logCameraImage(timestamp, bitmap, "jpg");
    }

    public void logCameraImage(long timestamp, Bitmap bitmap, String format) {
        if (!running.get()) return;
        try {
            File cameraDir = new File(logDirectory, "camera");
            if (!cameraDir.exists()) {
                boolean created = cameraDir.mkdirs();
                Log.d(TAG, "Created camera directory: " + cameraDir.getAbsolutePath() + ", success: " + created);
            }

            File imageFile = new File(cameraDir, timestamp + "." + format.toLowerCase());
            try (FileOutputStream fos = new FileOutputStream(imageFile)) {
                boolean success;

                if ("png".equals(format.toLowerCase())) {
                    success = bitmap.compress(Bitmap.CompressFormat.PNG, 100, fos);
                } else {
                    success = bitmap.compress(Bitmap.CompressFormat.JPEG, 95, fos);
                }

                Log.d(TAG, "Saved camera image: " + imageFile.getName() + ", success: " + success + ", size: " + imageFile.length() + " bytes");
            }
        } catch (IOException e) {
            Log.e(TAG, "Error saving camera image", e);
        }
    }

    public void logCameraIntrinsics(String intrinsicsData) {
        if (!running.get()) return;
        try {
            File cameraDir = new File(logDirectory, "camera");
            if (!cameraDir.exists()) {
                cameraDir.mkdirs();
            }

            File intrinsicsFile = new File(cameraDir, "intrinsics.txt");
            try (FileOutputStream fos = new FileOutputStream(intrinsicsFile)) {
                fos.write(intrinsicsData.getBytes());
                Log.d(TAG, "Saved camera intrinsics to " + intrinsicsFile.getName());
            }
        } catch (IOException e) {
            Log.e(TAG, "Error saving camera intrinsics", e);
        }
    }

    public boolean isRunning() {
        return running.get();
    }

    /**
     * Логирует ZMQMessage в bag файл
     */
    public void logZMQMessage(Messages.ZMQMessage message) {
        Log.d(TAG, "Logger.logZMQMessage called, running: " + running.get() + ", bagLogger: " + (bagLogger != null) + ", message: " + (message != null ? message.getTopic() : "null"));
        if (!running.get() || bagLogger == null || message == null) {
            Log.d(TAG, "Skipping logZMQMessage - running: " + running.get() + ", bagLogger: " + (bagLogger != null) + ", message: " + (message != null));
            return;
        }
        
        bagLogger.addMessage(message.getTopic(), message);
    }


    /**
     * Принудительно записывает буфер указанного топика
     */
    public void flushTopic(String topic) {
        if (bagLogger != null) {
            bagLogger.flushTopic(topic);
        }
    }

    /**
     * Получает статистику bag logger'а
     */
    public String getBagLoggerStats() {
        if (bagLogger != null) {
            return bagLogger.getBufferStats();
        }
        return "BagLogger not available";
    }

}
