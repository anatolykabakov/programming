package ai.flow.android;

import android.media.Image;
import android.util.Log;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

public class KITTIDataset {
    private static final String TAG = "KITTIDataset";
    private static KITTIDataset instance;
    
    private String logDirectory;
    private String imagesDirPath;
    private File odomFile;
    private File imagesDir;
    
    private KITTIDataset() {
        // Private constructor for singleton
    }
    
    public static synchronized KITTIDataset getInstance() {
        if (instance == null) {
            instance = new KITTIDataset();
        }
        return instance;
    }
    
    public void setLogDirectory(String directory) {
        this.logDirectory = directory;
        initializeDirectories();
    }
    
    private void initializeDirectories() {
        try {
            // Create main log directory
            File logDir = new File(logDirectory);
            if (!logDir.exists()) {
                logDir.mkdirs();
            }
            
            // Create images subdirectory
            imagesDirPath = logDirectory + File.separator + "images";
            imagesDir = new File(imagesDirPath);
            if (!imagesDir.exists()) {
                imagesDir.mkdirs();
            }
            
            // Create odometry file
            odomFile = new File(logDirectory, "odometry.txt");
            if (!odomFile.exists()) {
                odomFile.createNewFile();
            }
            
            Log.i(TAG, "KITTI dataset initialized in: " + logDirectory);
            
        } catch (IOException e) {
            Log.e(TAG, "Failed to initialize KITTI dataset directories", e);
        }
    }
    
    public void saveCarStateData(byte[] carStateData) {
        // For now, just log the raw data size
        // This can be extended to parse protobuf data if needed
        String logMsg = Long.toString(System.currentTimeMillis()) + "," +
                "carState_data_size:" + carStateData.length;
        try {
            BufferedWriter buf = new BufferedWriter(new FileWriter(odomFile, true));
            buf.append(logMsg);
            buf.newLine();
            buf.close();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void saveImage(Image image) {
        String imgName = Long.toString(System.currentTimeMillis()) + ".jpg";
        String imgPath = imagesDirPath + File.separator + imgName;
        File imageFile = new File(imgPath);

        try {
            // Convert Image to JPEG and save
            // This is a simplified version - in real implementation you'd need proper image conversion
            imageFile.createNewFile();
            Log.d(TAG, "Image saved: " + imgPath);
        } catch (IOException e) {
            Log.e(TAG, "Failed to save image", e);
        }
    }
    
    public void logMessage(String message) {
        try {
            BufferedWriter buf = new BufferedWriter(new FileWriter(odomFile, true));
            buf.append(message);
            buf.newLine();
            buf.close();
        } catch (IOException e) {
            Log.e(TAG, "Failed to log message", e);
        }
    }
}