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
import android.util.Log;

import java.io.File;

//import ai.flow.R;
//import ai.flow.android.R;

import ai.flow.openpilot.ServiceControlsd;
import ai.flow.openpilot.ServiceDebugd;
import ai.flow.openpilot.ServiceRadard;
//import ai.flow.openpilot.ServiceDebugd;

public class MainActivity extends AppCompatActivity {
    public static final String LOG_TAG = "MainActivity";
    private Button startAdasButton = null;
    private Button stopAdasButton = null;
    private Button startLoggingButton = null;
    private Button stopLoggingButton = null;
    private Button startJoystickButton = null;
    private Button stopJoystickButton = null;
    private Button selectDirectoryButton = null;
    private Button btnSteerLeft = null;
    private Button btnSteerRight = null;
    private Button btnAccel = null;
    private Button btnBrake = null;
    private Button btnAccEnable = null;
    private TextureView mImageView = null;
    private TextView logDirectoryPath = null;
    private TextView curvatureDisplay = null;

    CameraHandler cameraHandler = null;
//    PandaHandler pandaHandler = null;
    GPSHandler gpsHandler = null;
    IMUHandler imuHandler = null;
    
    // Camera state tracking
    private boolean cameraStarted = false;

    // SharedPreferences for storing log directory
    private SharedPreferences preferences;
    private static final String PREFS_NAME = "adas_prefs";

    @RequiresApi(api = Build.VERSION_CODES.M)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

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
        startLoggingButton = findViewById(R.id.startLoggingButton);
        stopLoggingButton = findViewById(R.id.stopLoggingButton);
        mImageView = findViewById(R.id.textureView);

        preferences = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        cameraHandler = new CameraHandler(getApplication().getApplicationContext(), mImageView);
        gpsHandler = new GPSHandler(this);
        imuHandler = new IMUHandler(this);
        Intent intent = new Intent(getApplicationContext(), AdasAppHandler.class);
        startService(intent);
        gpsHandler.start();
        imuHandler.start();

        startLoggingButton.setOnClickListener(v -> {
            Log.i(LOG_TAG, "Starting logging...");
            
            // Start camera only once
            if (!cameraStarted) {
                cameraHandler.start();
                cameraStarted = true;
                Log.i(LOG_TAG, "Camera started for the first time");
            } else {
                Log.i(LOG_TAG, "Camera already started, skipping camera start");
            }

            File externalDir = Environment.getExternalStorageDirectory();
            File logsDir = new File(externalDir, "adas_logs");
            Logger.getInstance().start(logsDir.getAbsolutePath());

            Log.i(LOG_TAG, "Logger started, isRunning: " + Logger.getInstance().isRunning());
            Log.i(LOG_TAG, "IMU Handler running: " + imuHandler.isRunning());
            Log.i(LOG_TAG, "GPS Handler running: " + gpsHandler.isRunning());

            startLoggingButton.setEnabled(false);
            stopLoggingButton.setEnabled(true);
            Toast.makeText(MainActivity.this, "📹 Logging Started", Toast.LENGTH_SHORT).show();
        });

        stopLoggingButton.setOnClickListener(v -> {
            Logger.getInstance().stop();

            startLoggingButton.setEnabled(true);
            stopLoggingButton.setEnabled(false);
            Toast.makeText(MainActivity.this, "⏹ Logging Stopped", Toast.LENGTH_SHORT).show();
        });

    }
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (cameraHandler != null) cameraHandler.stop();
        if (gpsHandler != null) gpsHandler.stop();
        if (imuHandler != null) imuHandler.stop();
        stopService(new Intent(getApplicationContext(), AdasAppHandler.class));
        
        // Reset camera state
        cameraStarted = false;
        
        android.util.Log.i("MainActivity", "All services stopped in onDestroy");
    }

}
