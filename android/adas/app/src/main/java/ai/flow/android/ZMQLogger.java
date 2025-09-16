package ai.flow.android;

import android.util.Log;
import org.zeromq.ZMQ;
import org.zeromq.ZContext;
import org.zeromq.ZMQ.Socket;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.Map;
import java.util.HashMap;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.TimeUnit;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicInteger;

// Video processing imports
import android.media.MediaRecorder;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.media.Image;
import android.media.ImageReader;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.view.Surface;
import java.nio.ByteBuffer;
import messaging.PortMap;

public class ZMQLogger {
    private static final String TAG = "ZMQLogger";
    
    // ZMQ configuration
    private PortMap portMap;
    
    // Topics to monitor
    private static final String[] TOPICS;
    
    static {
        String[] dataTopics = ZMQConfig.getDataTopics();
        TOPICS = new String[dataTopics.length + 1];
        System.arraycopy(dataTopics, 0, TOPICS, 0, dataTopics.length);
        TOPICS[dataTopics.length] = ZMQConfig.getCounterTopic();
    }
    
    // Static instance
    private static ZMQLogger instance;
    private static final Object lock = new Object();
    
    // ZMQ components
    private ZContext context;
    private Socket subscriber;
    private Socket counterSubscriber;
    private ExecutorService executor;
    private AtomicBoolean running = new AtomicBoolean(false);
    
    // Logging configuration
    private File logDirectory;
    private SimpleDateFormat dateFormat;
    private Map<String, FileOutputStream> logFiles;
    private Map<String, Long> messageCounts;
    
    // Video recording
    private MediaRecorder mediaRecorder;
    private boolean videoRecording;
    private int videoWidth = 1280;
    private int videoHeight = 720;
    private int videoFPS = 30;
    private String currentVideoFile;
    private File videoDirectory;
    
    // Text file writers
    private PrintWriter imuWriter;
    private PrintWriter gpsWriter;
    
    private ZMQLogger() {
        // Initialize collections first
        dateFormat = new SimpleDateFormat("yyyy-MM-dd_HH-mm-ss", Locale.getDefault());
        logFiles = new HashMap<>();
        messageCounts = new HashMap<>();
        
        // Load port map
        portMap = new PortMap().load();
        
        // Initialize logging with default directory
        setLogDirectory("/sdcard/adas_logs");
        
        // Initialize video recording
        videoRecording = false;
        currentVideoFile = null;
        mediaRecorder = null;
        videoDirectory = new File(logDirectory, "videos");
        if (!videoDirectory.exists()) {
            videoDirectory.mkdirs();
        }
        
        // Initialize message counts
        for (String topic : TOPICS) {
            messageCounts.put(topic, 0L);
        }
    }
    
    public static ZMQLogger getInstance() {
        if (instance == null) {
            synchronized (lock) {
                if (instance == null) {
                    instance = new ZMQLogger();
                }
            }
        }
        return instance;
    }
    
    public void start() {
        if (running.get()) {
            Log.i(TAG, "ZMQLogger already running");
            return;
        }
        
        Log.i(TAG, "Starting ZMQ logger...");
        Log.d(TAG, "Log directory: " + (logDirectory != null ? logDirectory.getAbsolutePath() : "null"));
        
        if (logDirectory == null) {
            Log.e(TAG, "Log directory is null! Cannot start logger.");
            return;
        }
        
        if (!logDirectory.exists()) {
            Log.w(TAG, "Log directory does not exist, creating: " + logDirectory.getAbsolutePath());
            boolean created = logDirectory.mkdirs();
            if (!created) {
                Log.e(TAG, "Failed to create log directory: " + logDirectory.getAbsolutePath());
                return;
            }
        }
        
        try {
            // Create ZMQ context
            context = new ZContext();
            
            // Create subscriber socket for data topics
            subscriber = context.createSocket(ZMQ.SUB);
            
            // Subscribe to data topics on fixed ports (same as handlers)
            subscriber.connect(ZMQConfig.getCameraEndpoint()); // Camera data
            subscriber.connect(ZMQConfig.getGPSEndpoint()); // GPS data  
            subscriber.connect(ZMQConfig.getIMUEndpoint()); // IMU data
            
            // Subscribe to all topics
            for (String topic : TOPICS) {
                if (!topic.equals(ZMQConfig.getCounterTopic())) {
                    subscriber.subscribe(topic);
                    Log.d(TAG, "Subscribed to topic: " + topic);
                }
            }
            
            // Create separate subscriber for counters (try different ports)
            counterSubscriber = context.createSocket(ZMQ.SUB);
            boolean counterConnected = false;
            for (int port = ZMQConfig.getCounterPortStart(); port <= ZMQConfig.getCounterPortEnd(); port++) {
                try {
                    String endpoint = ZMQConfig.getCounterEndpoint(port);
                    counterSubscriber.connect(endpoint);
                    counterSubscriber.subscribe(ZMQConfig.getCounterTopic());
                    Log.d(TAG, "Subscribed to counter topic on " + endpoint);
                    counterConnected = true;
                    break;
                } catch (Exception e) {
                    Log.w(TAG, "Failed to connect to counter port " + port + ", trying next...");
                }
            }
            
            if (!counterConnected) {
                Log.w(TAG, "Could not connect to any counter port");
            }
            
            // Start logger thread
            running.set(true);
            executor = Executors.newSingleThreadExecutor();
            executor.submit(this::loggerLoop);
            
            Log.i(TAG, "ZMQLogger started successfully");
            
        } catch (Exception e) {
            Log.e(TAG, "Error starting ZMQLogger", e);
            stop();
        }
    }
    
    public void stop() {
        if (!running.get()) {
            return;
        }
        
        Log.i(TAG, "Stopping ZMQ logger...");
        
        running.set(false);
        
        if (executor != null) {
            executor.shutdown();
            try {
                if (!executor.awaitTermination(5, TimeUnit.SECONDS)) {
                    executor.shutdownNow();
                }
            } catch (InterruptedException e) {
                executor.shutdownNow();
                Thread.currentThread().interrupt();
            }
        }
        
        // Stop video recording
        stopVideoRecording();
        
        // Close text file writers
        if (imuWriter != null) {
            imuWriter.close();
            imuWriter = null;
        }
        if (gpsWriter != null) {
            gpsWriter.close();
            gpsWriter = null;
        }
        
        // Close log files
        for (FileOutputStream fos : logFiles.values()) {
            try {
                fos.close();
            } catch (IOException e) {
                Log.e(TAG, "Error closing log file", e);
            }
        }
        logFiles.clear();
        
        if (subscriber != null) {
            subscriber.close();
        }
        
        if (counterSubscriber != null) {
            counterSubscriber.close();
        }
        
        if (context != null) {
            context.close();
        }
        
        Log.i(TAG, "ZMQLogger stopped");
    }
    
    private void loggerLoop() {
        Log.d(TAG, "Logger loop started");
        int loopCount = 0;
        
        while (running.get()) {
            try {
                loopCount++;
                if (loopCount % 100 == 0) {
                    Log.d(TAG, "Logger loop running, iteration: " + loopCount);
                }
                
                // Check data subscriber
                if (subscriber != null) {
                    String topic = subscriber.recvStr(ZMQ.DONTWAIT);
                    if (topic != null) {
                        byte[] data = subscriber.recv(ZMQ.DONTWAIT);
                        if (data != null) {
                            Log.i(TAG, "Received message on topic: " + topic + ", size: " + data.length);
                            processDataMessage(topic, data);
                        } else {
                            Log.w(TAG, "Received topic but no data: " + topic);
                        }
                    }
                } else {
                    Log.w(TAG, "Data subscriber is null");
                }
                
                // Check counter subscriber
                if (counterSubscriber != null) {
                    String topic = counterSubscriber.recvStr(ZMQ.DONTWAIT);
                    if (topic != null && topic.equals("messageCounters")) {
                        byte[] data = counterSubscriber.recv(ZMQ.DONTWAIT);
                        if (data != null) {
                            processCounterMessage(data);
                        }
                    }
                }
                
                // Small delay to prevent overwhelming
                Thread.sleep(10);
                
            } catch (Exception e) {
                Log.e(TAG, "Error in logger loop", e);
                break;
            }
        }
        
        Log.d(TAG, "Logger loop stopped");
    }
    
    private void processDataMessage(String topic, byte[] data) {
        try {
            // Update message count
            messageCounts.put(topic, messageCounts.get(topic) + 1);
            
            Log.d(TAG, "Processing " + topic + " message, size: " + data.length + 
                  " bytes, count: " + messageCounts.get(topic));
            
            // Log based on topic type
            switch (topic) {
                case "wideRoadCameraState":
                    logCameraState(data);
                    break;
                case "wideRoadCameraBuffer":
                    logCameraBuffer(data);
                    break;
                case "gpsLocation":
                case "gpsData":
                    Log.d(TAG, "Processing GPS data: " + topic);
                    logGPSData(topic, data);
                    break;
                case "imuData":
                case "accelerometerData":
                case "gyroscopeData":
                case "magnetometerData":
                    Log.d(TAG, "Processing IMU data: " + topic);
                    logIMUData(topic, data);
                    break;
                default:
                    Log.d(TAG, "Unknown topic: " + topic);
                    break;
            }
            
            Log.d(TAG, "Successfully processed " + topic + " message");
            
        } catch (Exception e) {
            Log.e(TAG, "Error processing data message for topic: " + topic, e);
        }
    }
    
    private void processCounterMessage(byte[] data) {
        try {
            String counterData = new String(data);
            logToFile("counters", counterData);
            Log.d(TAG, "Logged counter data: " + counterData);
        } catch (Exception e) {
            Log.e(TAG, "Error processing counter message", e);
        }
    }
    
    private void logCameraState(byte[] data) {
        // Log camera state metadata
        String timestamp = dateFormat.format(new Date());
        String logEntry = String.format("[%s] Camera State - Size: %d bytes\n", 
                                      timestamp, data.length);
        logToFile("camera_state", logEntry);
    }
    
    private void logCameraBuffer(byte[] data) {
        // Log camera buffer (image data)
        String timestamp = dateFormat.format(new Date());
        String logEntry = String.format("[%s] Camera Buffer - Size: %d bytes\n", 
                                      timestamp, data.length);
        logToFile("camera_buffer", logEntry);
        
        // Save image data to AVI video
        saveImageToVideo(data);
    }
    
    private void logGPSData(String topic, byte[] data) {
        String timestamp = dateFormat.format(new Date());
        String logEntry = String.format("[%s] %s - Size: %d bytes\n", 
                                      timestamp, topic, data.length);
        
        Log.d(TAG, "Logging GPS data: " + logEntry.trim());
        
        logToFile("gps_data", logEntry);
        
        // Save GPS data to text file with timestamp
        saveGPSToTextFile(data, System.currentTimeMillis());
        
        Log.d(TAG, "GPS data logged successfully");
    }
    
    private void logIMUData(String topic, byte[] data) {
        String timestamp = dateFormat.format(new Date());
        String logEntry = String.format("[%s] %s - Size: %d bytes\n", 
                                      timestamp, topic, data.length);
        
        Log.d(TAG, "Logging IMU data: " + logEntry.trim());
        
        logToFile("imu_data", logEntry);
        
        // Save IMU data to text file with timestamp
        saveIMUToTextFile(data, System.currentTimeMillis());
        
        Log.d(TAG, "IMU data logged successfully");
    }
    
    private void logToFile(String logType, String data) {
        try {
            FileOutputStream fos = logFiles.get(logType);
            if (fos == null) {
                File logFile = new File(logDirectory, logType + ".log");
                Log.i(TAG, "Creating new log file: " + logFile.getAbsolutePath());
                fos = new FileOutputStream(logFile, true);
                logFiles.put(logType, fos);
            }
            
            fos.write(data.getBytes());
            fos.flush();
            Log.d(TAG, "Successfully wrote to " + logType + ".log: " + data.trim());
            
        } catch (IOException e) {
            Log.e(TAG, "Error writing to log file: " + logType, e);
        }
    }
    
    private void saveImageData(byte[] data, String timestamp) {
        try {
            File imageFile = new File(logDirectory, "image_" + timestamp + ".bin");
            FileOutputStream fos = new FileOutputStream(imageFile);
            fos.write(data);
            fos.close();
            Log.d(TAG, "Saved image data to: " + imageFile.getAbsolutePath());
        } catch (IOException e) {
            Log.e(TAG, "Error saving image data", e);
        }
    }
    
    // Get statistics
    public Map<String, Long> getMessageCounts() {
        return new HashMap<>(messageCounts);
    }
    
    public String getLogDirectory() {
        return logDirectory.getAbsolutePath();
    }
    
    public void setLogDirectory(String directoryPath) {
        // Close existing log files
        if (logFiles != null) {
            for (FileOutputStream fos : logFiles.values()) {
                try {
                    fos.close();
                } catch (IOException e) {
                    Log.e(TAG, "Error closing log file", e);
                }
            }
            logFiles.clear();
        }
        
        // Set new directory
        logDirectory = new File(directoryPath);
        if (!logDirectory.exists()) {
            boolean created = logDirectory.mkdirs();
            if (!created) {
                Log.e(TAG, "Failed to create log directory: " + directoryPath);
            }
        }
        
        // Create video subdirectory
        videoDirectory = new File(logDirectory, "videos");
        if (!videoDirectory.exists()) {
            boolean created = videoDirectory.mkdirs();
            if (!created) {
                Log.e(TAG, "Failed to create video directory: " + videoDirectory.getPath());
            }
        }
        
        // Initialize text file writers
        initializeTextWriters();
        
        Log.i(TAG, "Log directory set to: " + logDirectory.getAbsolutePath());
    }
    
    // Static convenience methods
    public static void startStatic() {
        getInstance().start();
    }
    
    public static void stopStatic() {
        getInstance().stop();
    }
    
    public static Map<String, Long> getStatistics() {
        return getInstance().getMessageCounts();
    }
    
    public static String getLogPath() {
        return getInstance().getLogDirectory();
    }
    
    // Initialize text file writers
    private void initializeTextWriters() {
        try {
            // Close existing writers
            if (imuWriter != null) {
                imuWriter.close();
            }
            if (gpsWriter != null) {
                gpsWriter.close();
            }
            
            // Create new writers
            File imuFile = new File(logDirectory, "imu_data.txt");
            File gpsFile = new File(logDirectory, "gps_data.txt");
            
            imuWriter = new PrintWriter(new FileOutputStream(imuFile, true));
            gpsWriter = new PrintWriter(new FileOutputStream(gpsFile, true));
            
            Log.i(TAG, "Text file writers initialized");
        } catch (IOException e) {
            Log.e(TAG, "Error initializing text writers", e);
        }
    }
    
    // Save image data to video file
    private void saveImageToVideo(byte[] imageData) {
        try {
            // Start video recording if not already started
            if (!videoRecording) {
                startVideoRecording();
            }
            
            // For now, we'll save the raw YUV data
            // In a real implementation, you would need to convert YUV to frames
            // and feed them to MediaRecorder
            Log.d(TAG, "Received camera frame, size: " + imageData.length + " bytes");
            
        } catch (Exception e) {
            Log.e(TAG, "Error processing image for video", e);
        }
    }
    
    // Start video recording using MediaRecorder
    private void startVideoRecording() {
        try {
            if (videoRecording) {
                return; // Already recording
            }
            
            String videoFileName = "camera_" + dateFormat.format(new Date()) + ".mp4";
            currentVideoFile = new File(videoDirectory, videoFileName).getAbsolutePath();
            
            mediaRecorder = new MediaRecorder();
            mediaRecorder.setVideoSource(MediaRecorder.VideoSource.SURFACE);
            mediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4);
            mediaRecorder.setOutputFile(currentVideoFile);
            mediaRecorder.setVideoEncoder(MediaRecorder.VideoEncoder.H264);
            mediaRecorder.setVideoSize(videoWidth, videoHeight);
            mediaRecorder.setVideoFrameRate(videoFPS);
            
            mediaRecorder.prepare();
            mediaRecorder.start();
            
            videoRecording = true;
            
            Log.i(TAG, "Video recording started: " + currentVideoFile);
        } catch (Exception e) {
            Log.e(TAG, "Error starting video recording", e);
            videoRecording = false;
        }
    }
    
    // Stop video recording
    private void stopVideoRecording() {
        try {
            if (videoRecording && mediaRecorder != null) {
                mediaRecorder.stop();
                mediaRecorder.release();
                mediaRecorder = null;
                videoRecording = false;
                Log.i(TAG, "Video recording stopped: " + currentVideoFile);
            }
        } catch (Exception e) {
            Log.e(TAG, "Error stopping video recording", e);
        }
    }
    
    // Save IMU data to text file
    private void saveIMUToTextFile(byte[] data, long timestamp) {
        try {
            if (imuWriter != null) {
                // Parse IMU data (simplified - assumes specific format)
                String dataString = new String(data, "UTF-8");
                imuWriter.printf("%d %s\n", timestamp, dataString);
                imuWriter.flush();
                Log.d(TAG, "IMU data saved to text file: " + dataString.length() + " chars");
            } else {
                Log.w(TAG, "IMU writer is null, cannot save to text file");
            }
        } catch (Exception e) {
            Log.e(TAG, "Error saving IMU data to text file", e);
        }
    }
    
    // Save GPS data to text file
    private void saveGPSToTextFile(byte[] data, long timestamp) {
        try {
            if (gpsWriter != null) {
                // Parse GPS data (simplified - assumes specific format)
                String dataString = new String(data, "UTF-8");
                gpsWriter.printf("%d %s\n", timestamp, dataString);
                gpsWriter.flush();
                Log.d(TAG, "GPS data saved to text file: " + dataString.length() + " chars");
            } else {
                Log.w(TAG, "GPS writer is null, cannot save to text file");
            }
        } catch (Exception e) {
            Log.e(TAG, "Error saving GPS data to text file", e);
        }
    }
}
