package ai.flow.adas;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.view.TextureView;
import android.widget.Button;
import android.widget.Toast;
import android.content.SharedPreferences;
import android.util.Log;

import java.io.File;

import ai.flow.adas.vision.LaneOverlayView;
import ai.flow.adas.vision.VisionPipeline;

public class MainActivity extends AppCompatActivity {
    public static final String LOG_TAG = "MainActivity";
    private static final int REQ_PERMISSIONS = 1;

    private Button startLoggingButton = null;
    private Button stopLoggingButton = null;
    private TextureView mImageView = null;

    CameraHandler cameraHandler = null;
    GPSHandler gpsHandler = null;
    IMUHandler imuHandler = null;
    VisionPipeline visionPipeline = null;
    LaneOverlayView laneOverlay = null;

    private boolean cameraStarted = false;
    private boolean storagePromptShown = false;

    private SharedPreferences preferences;
    private static final String PREFS_NAME = "adas_prefs";

    @RequiresApi(api = Build.VERSION_CODES.M)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        startLoggingButton = findViewById(R.id.startLoggingButton);
        stopLoggingButton = findViewById(R.id.stopLoggingButton);
        mImageView = findViewById(R.id.textureView);
        laneOverlay = findViewById(R.id.laneOverlay);

        preferences = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        cameraHandler = new CameraHandler(getApplication().getApplicationContext(), mImageView);
        gpsHandler = new GPSHandler(this);
        imuHandler = new IMUHandler(this);

        AdasConfig config = AdasConfig.load(this);
        laneOverlay.setIntrinsics(
                config.fx, config.fy, config.cx, config.cy,
                cameraHandler.W > 0 ? cameraHandler.W : config.frameW,
                cameraHandler.H > 0 ? cameraHandler.H : config.frameH);
        laneOverlay.setCameraHeight(config.camZ);
        laneOverlay.setWaypointShift(config.camX);
        laneOverlay.setSteerRatio(config.steerRatio);

        // PP / steer / live calib HUD from native via ZMQ OUT (:5556)
        ZMQBridgeService.setOutboundListener((topic, msg) -> {
            if (laneOverlay == null) {
                return;
            }
            if (msg.hasLaneKeep()) {
                LaneKeepOuter.LaneKeepState lk = msg.getLaneKeep();
                laneOverlay.setLaneKeep(
                        lk.getHasTarget(),
                        (float) lk.getTargetX(),
                        (float) lk.getTargetY(),
                        (float) lk.getLookaheadM(),
                        (float) lk.getCurvature(),
                        (float) lk.getSteerRad(),
                        lk.getStatus());
            }
            if (msg.hasSteerCommand()) {
                SteerOuter.SteerCommand sc = msg.getSteerCommand();
                laneOverlay.setSteerCommand(sc.getTorqueCnm(), sc.getEnabled());
            }
            if (msg.hasCameraCalib()) {
                CameraCalibOuter.CameraCalibrationState c = msg.getCameraCalib();
                laneOverlay.setCameraCalib(
                        (float) c.getPitchDeg(),
                        (float) c.getYawDeg(),
                        (float) c.getRollDeg(),
                        (float) c.getCameraHeightM(),
                        c.getCalibrationSuccess(),
                        c.getCalPercent() > 0 ? c.getCalPercent()
                                : Math.min(100, c.getNUpdates() * 20));
            }
        });

        // ONNX first: with Panda plugged in, AdasAppHandler immediately starts native
        // (USB fd + panda + protobuf) and concurrent createSession OOMs / fails.
        if (config.visionSupercombo) {
            try {
                visionPipeline = new VisionPipeline(this, laneOverlay, config.cameraCalib);
                cameraHandler.setVisionPipeline(visionPipeline);
                cameraHandler.setLaneOverlay(laneOverlay);
                Log.i(LOG_TAG, "VisionPipeline ready (supercombo ONNX, posePub="
                        + config.cameraCalib + ")");
            } catch (Exception e) {
                Log.e(LOG_TAG, "VisionPipeline init failed", e);
                String detail = e.getClass().getSimpleName() + ": " + e.getMessage();
                if (e.getCause() != null) {
                    detail += " | cause=" + e.getCause().getMessage();
                }
                Toast.makeText(this, "ONNX init failed: " + detail, Toast.LENGTH_LONG).show();
            }
        } else {
            Log.i(LOG_TAG, "vision_supercombo disabled in assets/config.json");
        }

        // Panda / ZMQ after model is in memory
        startService(new Intent(getApplicationContext(), AdasAppHandler.class));

        mImageView.setSurfaceTextureListener(new TextureView.SurfaceTextureListener() {
            @Override public void onSurfaceTextureAvailable(android.graphics.SurfaceTexture surface, int w, int h) {
                Log.i(LOG_TAG, "TextureView available " + w + "x" + h);
                startCameraIfNeeded(w, h);
            }
            @Override public void onSurfaceTextureSizeChanged(android.graphics.SurfaceTexture surface, int w, int h) {
                cameraHandler.configurePreviewTransform(w, h);
            }
            @Override public boolean onSurfaceTextureDestroyed(android.graphics.SurfaceTexture surface) {
                Log.i(LOG_TAG, "TextureView destroyed — releasing camera");
                stopCamera();
                return true;
            }
            @Override public void onSurfaceTextureUpdated(android.graphics.SurfaceTexture surface) {}
        });

        requestRuntimePermissionsIfNeeded();

        startLoggingButton.setOnClickListener(v -> {
            Log.i(LOG_TAG, "Starting logging...");
            maybePromptStorageAccess();
            startCameraIfNeeded(mImageView.getWidth(), mImageView.getHeight());

            File externalDir = Environment.getExternalStorageDirectory();
            File logsDir = new File(externalDir, "adas_logs");
            Logger.getInstance().start(logsDir.getAbsolutePath());
            if (cameraHandler != null) {
                cameraHandler.ensureBagIntrinsicsLogged();
            }

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
    protected void onResume() {
        super.onResume();
        if (gpsHandler != null) {
            gpsHandler.start();
        }
        if (imuHandler != null) {
            imuHandler.start();
        }
        if (mImageView != null && mImageView.isAvailable()) {
            startCameraIfNeeded(mImageView.getWidth(), mImageView.getHeight());
        }
    }

    @Override
    protected void onPause() {
        // Keep camera while briefly covered only if surface stays; surface destroy handles full release.
        super.onPause();
    }

    private void requestRuntimePermissionsIfNeeded() {
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED
                || ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED
                || ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED
                || ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED
                || (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S
                    && ContextCompat.checkSelfPermission(this, Manifest.permission.HIGH_SAMPLING_RATE_SENSORS)
                        != PackageManager.PERMISSION_GRANTED)) {
            java.util.ArrayList<String> perms = new java.util.ArrayList<>();
            perms.add(Manifest.permission.CAMERA);
            perms.add(Manifest.permission.WRITE_EXTERNAL_STORAGE);
            perms.add(Manifest.permission.ACCESS_FINE_LOCATION);
            perms.add(Manifest.permission.ACCESS_COARSE_LOCATION);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                perms.add(Manifest.permission.HIGH_SAMPLING_RATE_SENSORS);
            }
            ActivityCompat.requestPermissions(this, perms.toArray(new String[0]), REQ_PERMISSIONS);
        }
    }

    /** Do not open Settings from onCreate — it pauses the activity and aborts camera open. */
    private void maybePromptStorageAccess() {
        if (storagePromptShown) {
            return;
        }
        if (Build.VERSION.SDK_INT >= 30 && !Environment.isExternalStorageManager()) {
            storagePromptShown = true;
            Toast.makeText(this, "Grant All files access for bag logging", Toast.LENGTH_LONG).show();
            try {
                Intent getpermission = new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
                startActivity(getpermission);
            } catch (Exception e) {
                Log.e(LOG_TAG, "Cannot open storage settings", e);
            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode != REQ_PERMISSIONS) {
            return;
        }
        boolean cameraOk = checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED;
        Log.i(LOG_TAG, "Permissions result: cameraOk=" + cameraOk);
        if (cameraOk && mImageView != null && mImageView.isAvailable()) {
            startCameraIfNeeded(mImageView.getWidth(), mImageView.getHeight());
        } else if (!cameraOk) {
            Toast.makeText(this, "Camera permission required", Toast.LENGTH_LONG).show();
        }
        if (gpsHandler != null) {
            gpsHandler.start();
        }
        if (imuHandler != null) {
            imuHandler.start();
        }
    }

    private void startCameraIfNeeded(int w, int h) {
        if (cameraHandler == null || mImageView == null) {
            return;
        }
        if (w > 0 && h > 0) {
            cameraHandler.configurePreviewTransform(w, h);
        }
        if (cameraHandler.isCameraOpen()) {
            cameraStarted = true;
            return;
        }
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            Log.w(LOG_TAG, "Camera permission not granted yet");
            return;
        }
        if (!mImageView.isAvailable()) {
            Log.w(LOG_TAG, "TextureView surface not available yet");
            return;
        }
        Log.i(LOG_TAG, "Starting camera preview " + w + "x" + h);
        cameraStarted = false;
        cameraHandler.start();
        // cameraStarted set true from onResume retry once CameraHandler.isCameraOpen()
        cameraStarted = true; // prevent duplicate openCamera while callback pending
    }

    private void stopCamera() {
        if (cameraHandler != null) {
            cameraHandler.stop();
        }
        cameraStarted = false;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        ZMQBridgeService.setOutboundListener(null);
        if (visionPipeline != null) visionPipeline.close();
        stopCamera();
        if (gpsHandler != null) gpsHandler.stop();
        if (imuHandler != null) imuHandler.stop();
        stopService(new Intent(getApplicationContext(), AdasAppHandler.class));
        Log.i(LOG_TAG, "All services stopped in onDestroy");
    }
}
