package ai.flow.android;

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
import java.util.HashMap;

import androidx.annotation.Nullable;

class AdasAppInstance implements Runnable {
    private int fd;
    public AdasAppInstance(int fd) {
        this.fd = fd;
    }
    @Override
    public void run(){
        AdasAppHandler.nativeStart(this.fd);
    }
}

public class AdasAppHandler extends Service {
    String TAG = "AdasAppHandler";

    private static final String ACTION_USB_PERMISSION = "ai.flow.android.USB_PERMISSION";

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
                    UsbManager usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
                    HashMap<String, UsbDevice> deviceList = usbManager.getDeviceList();
                    Log.i(TAG, "Number of USB devices found: "+deviceList.size());
                    for (UsbDevice usbDevice : deviceList.values())
                    {
                        UsbDeviceConnection usbDeviceConnection = usbManager.openDevice(usbDevice);
                        if (usbDeviceConnection != null)
                        {
                            int fd = usbDeviceConnection.getFileDescriptor();
                            System.out.println("USB Device FD: " + fd);
                            AdasAppInstance adasAppInstance = new AdasAppInstance(fd);
                            new Thread(adasAppInstance).start();
                        }
                    }
                }
            }
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "AdasAppHandler created");

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

        // Request permission for already plugged Panda devices
        UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);
        HashMap<String, UsbDevice> deviceList = manager.getDeviceList();
        Log.i(TAG, "Number of USB devices found: "+deviceList.size());
        for (UsbDevice usbDevice : deviceList.values())
        {
            maybeRequestUSBPermission(usbDevice, this);
        }

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

        Intent zmqIntent = new Intent(this, ZMQBridgeService.class);
        stopService(zmqIntent);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private void maybeRequestUSBPermission(UsbDevice usbDevice, Context context) {
        if (usbDevice == null) {
            Log.w(TAG, "USB device is null");
            return;
        }

        UsbManager usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        if (usbManager.hasPermission(usbDevice)) {
            Log.i(TAG, "USB permission already granted for device: " + usbDevice.getDeviceName());
            UsbDeviceConnection usbDeviceConnection = usbManager.openDevice(usbDevice);
            if (usbDeviceConnection != null) {
                int fd = usbDeviceConnection.getFileDescriptor();
                System.out.println("USB Device FD: " + fd);
                AdasAppInstance adasAppInstance = new AdasAppInstance(fd);
                new Thread(adasAppInstance).start();
            }
        } else {
            Log.i(TAG, "Requesting USB permission for device: " + usbDevice.getDeviceName());
            PendingIntent permissionIntent = PendingIntent.getBroadcast(context, 0, new Intent(ACTION_USB_PERMISSION), PendingIntent.FLAG_IMMUTABLE);
            usbManager.requestPermission(usbDevice, permissionIntent);
        }
    }

    // Native method declarations
    public static native void nativeStart(int fd);

    static {
        System.loadLibrary("adas_app_android");
    }
}
