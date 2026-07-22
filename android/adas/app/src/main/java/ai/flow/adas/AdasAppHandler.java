package ai.flow.adas;

import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.util.Log;
import android.os.IBinder;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.HashMap;

class AdasAppInstance implements Runnable {
    private final int fd;
    private final String dbcPath;
    private final String configPath;

    public AdasAppInstance(int fd, String dbcPath, String configPath) {
        this.fd = fd;
        this.dbcPath = dbcPath;
        this.configPath = configPath;
    }

    @Override
    public void run() {
        if (!AdasAppHandler.nativeLoaded) {
            Log.w("AdasAppHandler", "Skipping nativeStart: libadas_app_android.so not loaded");
            return;
        }
        try {
            AdasAppHandler.nativeStart(this.fd, this.dbcPath, this.configPath);
        } catch (Throwable t) {
            Log.e("AdasAppHandler", "nativeStart failed (fd=" + fd + ")", t);
        }
    }
}

public class AdasAppHandler extends Service {
    String TAG = "AdasAppHandler";
    /** False when CMake/native build is disabled or .so missing — service still runs for USB/ZMQ. */
    public static final boolean nativeLoaded;

    private static final String ACTION_USB_PERMISSION = "ai.flow.adas.USB_PERMISSION";
    private static final String DBC_ASSET = "vw_mqb_2010.dbc";
    private static final String CONFIG_ASSET = "config.json";
    /** comma.ai panda USB IDs */
    private static final int PANDA_VID = 0xbbaa;
    private static final int PANDA_PID = 0xddcc;

    private String dbcPath;
    private String configPath;
    private AdasConfig adasConfig;

    /**
     * Must keep this alive for the lifetime of native USB use. If GC finalizes it,
     * Android closes the FD and libusb control OUT/IN break.
     */
    private UsbDeviceConnection pandaConnection;
    private boolean nativeStarted;

    private BroadcastReceiver usbReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            System.out.println("RECEIVING INTENT: " + action);

            if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
                synchronized (this) {
                    UsbDevice usbDevice = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                    maybeRequestUSBPermission(usbDevice, context);
                }
            } else if (ACTION_USB_PERMISSION.equals(action)) {
                synchronized (this) {
                    UsbDevice usbDevice = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                    maybeRequestUSBPermission(usbDevice, context);
                }
            }
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "AdasAppHandler created (nativeLoaded=" + nativeLoaded + ")");

        dbcPath = ensureDbcAsset(this);
        Log.i(TAG, "DBC path: " + dbcPath);
        configPath = ensureConfigAsset(this);
        Log.i(TAG, "Config path: " + configPath);
        adasConfig = AdasConfig.load(this);
        Log.i(TAG, "Config lane_keep=" + adasConfig.laneKeep
                + " localization=" + adasConfig.localization
                + " camera_calib=" + adasConfig.cameraCalib);

        // Register USB receiver
        IntentFilter filter = new IntentFilter();
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(ACTION_USB_PERMISSION);

        // For Android 14+ (API 34+), we need to specify RECEIVER_EXPORTED or RECEIVER_NOT_EXPORTED
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            registerReceiver(usbReceiver, filter, Context.RECEIVER_EXPORTED);
        } else {
            registerReceiver(usbReceiver, filter);
        }

        // Request permission / open already-plugged Panda after a short delay so ONNX
        // session creation (MainActivity) can finish without RAM contention.
        new android.os.Handler(android.os.Looper.getMainLooper()).postDelayed(() -> {
            UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);
            if (manager == null) {
                return;
            }
            HashMap<String, UsbDevice> deviceList = manager.getDeviceList();
            Log.i(TAG, "Number of USB devices found: " + deviceList.size());
            for (UsbDevice usbDevice : deviceList.values()) {
                maybeRequestUSBPermission(usbDevice, this);
            }
        }, 1500);

        // Start ZMQ Bridge Service
        Intent zmqIntent = new Intent(this, ZMQBridgeService.class);
        startService(zmqIntent);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "AdasAppHandler starting...");
        return START_STICKY; // Restart service if killed
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "AdasAppHandler destroying...");

        if (usbReceiver != null) {
            unregisterReceiver(usbReceiver);
        }

        if (pandaConnection != null) {
            try {
                pandaConnection.close();
            } catch (Exception e) {
                Log.w(TAG, "Error closing panda USB connection", e);
            }
            pandaConnection = null;
        }

        if (nativeStarted) {
            try {
                nativeStop();
            } catch (UnsatisfiedLinkError e) {
                Log.w(TAG, "nativeStop unavailable", e);
            }
            nativeStarted = false;
        }

        Intent zmqIntent = new Intent(this, ZMQBridgeService.class);
        stopService(zmqIntent);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private static boolean isPanda(UsbDevice device) {
        return device != null && device.getVendorId() == PANDA_VID && device.getProductId() == PANDA_PID;
    }

    private void maybeRequestUSBPermission(UsbDevice usbDevice, Context context) {
        if (!isPanda(usbDevice)) {
            if (usbDevice != null) {
                Log.d(TAG, "Skipping non-panda USB device VID=0x"
                        + Integer.toHexString(usbDevice.getVendorId())
                        + " PID=0x" + Integer.toHexString(usbDevice.getProductId()));
            }
            return;
        }

        UsbManager usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        if (usbManager.hasPermission(usbDevice)) {
            Log.i(TAG, "USB permission already granted for panda: " + usbDevice.getDeviceName());
            openPandaAndStartNative(usbManager, usbDevice);
        } else {
            Log.i(TAG, "Requesting USB permission for panda: " + usbDevice.getDeviceName());
            PendingIntent permissionIntent = PendingIntent.getBroadcast(context, 0, new Intent(ACTION_USB_PERMISSION), PendingIntent.FLAG_IMMUTABLE);
            usbManager.requestPermission(usbDevice, permissionIntent);
        }
    }

    private synchronized void openPandaAndStartNative(UsbManager usbManager, UsbDevice usbDevice) {
        if (nativeStarted) {
            Log.i(TAG, "Native panda already started — skip duplicate open");
            return;
        }
        UsbDeviceConnection conn = usbManager.openDevice(usbDevice);
        if (conn == null) {
            Log.e(TAG, "openDevice returned null for panda");
            return;
        }
        // Keep reference so GC does not finalize/close the FD under native libusb.
        pandaConnection = conn;
        int fd = conn.getFileDescriptor();
        Log.i(TAG, "USB Device FD: " + fd + " (connection retained; safety/heartbeat in native PandaService)");

        nativeStarted = true;
        new Thread(new AdasAppInstance(fd, dbcPath, configPath), "AdasNative").start();
    }

    /** Copy vw_mqb_2010.dbc from APK assets to filesDir so native code can fopen it. */
    static String ensureDbcAsset(Context context) {
        return copyAssetToFiles(context, DBC_ASSET);
    }

    /** Copy config.json from APK assets to filesDir for native AdasApp. */
    static String ensureConfigAsset(Context context) {
        return copyAssetToFiles(context, CONFIG_ASSET);
    }

    static String copyAssetToFiles(Context context, String assetName) {
        File out = new File(context.getFilesDir(), assetName);
        // Always refresh config.json so APK asset edits apply without reinstall wipe.
        final boolean force = "config.json".equals(assetName);
        if (!force && out.exists() && out.length() > 0) {
            return out.getAbsolutePath();
        }
        try (InputStream in = context.getAssets().open(assetName);
             FileOutputStream fos = new FileOutputStream(out)) {
            byte[] buf = new byte[8192];
            int n;
            while ((n = in.read(buf)) > 0) {
                fos.write(buf, 0, n);
            }
            Log.i("AdasAppHandler", "Copied asset " + assetName + " -> " + out.getAbsolutePath());
            return out.getAbsolutePath();
        } catch (Exception e) {
            Log.e("AdasAppHandler", "Failed to copy asset " + assetName, e);
            return out.exists() ? out.getAbsolutePath() : "";
        }
    }

    // Native method declarations (only call if nativeLoaded)
    public static native void nativeStart(int fd, String dbcPath, String configPath);
    public static native void nativeStop();

    static {
        boolean ok = false;
        try {
            System.loadLibrary("adas_app_android");
            ok = true;
            Log.i("AdasAppHandler", "Loaded libadas_app_android.so");
        } catch (UnsatisfiedLinkError e) {
            // externalNativeBuild is commented out in app/build.gradle — vision/ONNX path still works
            Log.e("AdasAppHandler", "libadas_app_android.so missing; native panda path disabled", e);
        }
        nativeLoaded = ok;
    }
}
