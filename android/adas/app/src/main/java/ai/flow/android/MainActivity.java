package ai.flow.android;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;
import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.view.TextureView;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import android.content.SharedPreferences;

import java.io.File;

//import ai.flow.R;
//import ai.flow.android.R;

import ai.flow.openpilot.ServiceControlsd;
import ai.flow.openpilot.ServiceDebugd;
import ai.flow.openpilot.ServiceRadard;
//import ai.flow.openpilot.ServiceDebugd;

public class MainActivity extends AppCompatActivity {
    public static final String LOG_TAG = "MainActivity";
    private Button startButton = null;
    private Button stopButton = null;
    private Button selectDirectoryButton = null;
    private TextureView mImageView = null;
    private TextView logDirectoryPath = null;

    CameraHandler cameraHandler = null;
    PandaHandler pandaHandler = null;
    GPSHandler gpsHandler = null;
    IMUHandler imuHandler = null;
    ZMQLogger zmqLogger = null;
    
    // SharedPreferences for storing log directory
    private SharedPreferences preferences;
    private static final String PREFS_NAME = "adas_prefs";
    private static final String LOG_DIRECTORY_KEY = "log_directory";
    private static final String DEFAULT_LOG_DIRECTORY = "/sdcard/adas_logs";

    @RequiresApi(api = Build.VERSION_CODES.M)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        // Initialize ZMQ configuration
//        ZMQConfig.init(this);

        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED
                ||
                (ContextCompat.checkSelfPermission(MainActivity.this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED)
                ||
                (ContextCompat.checkSelfPermission(MainActivity.this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED)
                ||
                (ContextCompat.checkSelfPermission(MainActivity.this, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED)
                ||
                (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S && 
                 ContextCompat.checkSelfPermission(MainActivity.this, Manifest.permission.HIGH_SAMPLING_RATE_SENSORS) != PackageManager.PERMISSION_GRANTED)
        )
        {
            String[] permissions = {
                Manifest.permission.CAMERA,
                Manifest.permission.WRITE_EXTERNAL_STORAGE,
                Manifest.permission.ACCESS_FINE_LOCATION,
                Manifest.permission.ACCESS_COARSE_LOCATION
            };
            
            // Add HIGH_SAMPLING_RATE_SENSORS permission for Android 12+ (API 31+)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                String[] permissionsWithSensors = new String[permissions.length + 1];
                System.arraycopy(permissions, 0, permissionsWithSensors, 0, permissions.length);
                permissionsWithSensors[permissions.length] = Manifest.permission.HIGH_SAMPLING_RATE_SENSORS;
                permissions = permissionsWithSensors;
            }
            
            requestPermissions(permissions, 1);
        }
        if (Build.VERSION.SDK_INT >= 30){
            if (!Environment.isExternalStorageManager()){
                Intent getpermission = new Intent();
                getpermission.setAction(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
                startActivity(getpermission);
            }
        }

        stopButton =  findViewById(R.id.stopButton);
        startButton =  findViewById(R.id.startButton);
        selectDirectoryButton = findViewById(R.id.select_directory_button);
        mImageView = findViewById(R.id.textureView);
        logDirectoryPath = findViewById(R.id.log_directory_path);
        
        // Initialize SharedPreferences
        preferences = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        
        // Load saved log directory
        String savedLogDirectory = preferences.getString(LOG_DIRECTORY_KEY, DEFAULT_LOG_DIRECTORY);
        logDirectoryPath.setText(savedLogDirectory);

        cameraHandler = new CameraHandler(getApplication().getApplicationContext(), mImageView);
        gpsHandler = new GPSHandler(this);
        imuHandler = new IMUHandler(this);
//        zmqLogger = ZMQLogger.getInstance();
        
        // Set log directory for ZMQLogger
//        zmqLogger.setLogDirectory(savedLogDirectory);

        selectDirectoryButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                showDirectorySelectionDialog();
            }
        });

        startButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                cameraHandler.start();
                gpsHandler.start();
                imuHandler.start();
                AdasAppHandler.startStatic();
//                zmqLogger.start();
//                Intent intent = new Intent(getApplicationContext(), PandaHandler.class);
//                startService(intent);

                // ServiceDebugd.prepare(getApplication().getApplicationContext());
                // ServiceDebugd.start(getApplication().getApplicationContext(), "");
                // ServiceControlsd.prepare(getApplication().getApplicationContext());
                // ServiceControlsd.start(getApplication().getApplicationContext(), "");
                // ServiceRadard.prepare(getApplication().getApplicationContext());
                // ServiceRadard.start(getApplication().getApplicationContext(), "");
            }
        });

        stopButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                cameraHandler.stop();
                gpsHandler.stop();
                imuHandler.stop();
                AdasAppHandler.stopStatic();
//                zmqLogger.stop();
                stopService(new Intent(getApplicationContext(), PandaHandler.class));
            }
        });
    }
    
    private void showDirectorySelectionDialog() {
        // Create a simple dialog with common directory options
        String[] directories = {
            "/sdcard/adas_logs",
            "/sdcard/Download/adas_logs", 
            "/sdcard/Documents/adas_logs",
            "/sdcard/Android/data/ai.flow.android/files/logs",
            "/storage/emulated/0/adas_logs"
        };
        
        androidx.appcompat.app.AlertDialog.Builder builder = new androidx.appcompat.app.AlertDialog.Builder(this);
        builder.setTitle("Select Log Directory");
        builder.setItems(directories, (dialog, which) -> {
            String selectedDirectory = directories[which];
            setLogDirectory(selectedDirectory);
        });
        
        // Add custom option
        builder.setNeutralButton("Custom Path", (dialog, which) -> {
            showCustomPathDialog();
        });
        
        builder.setNegativeButton("Cancel", null);
        builder.show();
    }
    
    private void showCustomPathDialog() {
        androidx.appcompat.app.AlertDialog.Builder builder = new androidx.appcompat.app.AlertDialog.Builder(this);
        builder.setTitle("Enter Custom Path");
        
        final android.widget.EditText input = new android.widget.EditText(this);
        input.setText(logDirectoryPath.getText().toString());
        input.setHint("/sdcard/adas_logs");
        builder.setView(input);
        
        builder.setPositiveButton("OK", (dialog, which) -> {
            String customPath = input.getText().toString().trim();
            if (!customPath.isEmpty()) {
                setLogDirectory(customPath);
            }
        });
        
        builder.setNegativeButton("Cancel", null);
        builder.show();
    }
    
    private void setLogDirectory(String directory) {
        // Validate directory path
        if (isValidDirectory(directory)) {
            // Save to preferences
            preferences.edit().putString(LOG_DIRECTORY_KEY, directory).apply();
            
            // Update UI
            logDirectoryPath.setText(directory);
            
            // Update ZMQLogger
            zmqLogger.setLogDirectory(directory);
            
            Toast.makeText(this, "Log directory set to: " + directory, Toast.LENGTH_SHORT).show();
        } else {
            Toast.makeText(this, "Invalid directory path. Please check permissions.", Toast.LENGTH_LONG).show();
        }
    }
    
    private boolean isValidDirectory(String path) {
        try {
            File dir = new File(path);
            if (!dir.exists()) {
                // Try to create the directory
                return dir.mkdirs();
            }
            return dir.isDirectory() && dir.canWrite();
        } catch (Exception e) {
            return false;
        }
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

}