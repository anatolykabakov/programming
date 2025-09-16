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
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;

import androidx.annotation.Nullable;

import org.capnproto.StructList;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.concurrent.Flow;

import ai.flow.definitions.CarDefinitions;
import ai.flow.definitions.Definitions;

import messaging.ZMQSubHandler;

class PandaInstance implements Runnable {
    private int fd;
    public PandaInstance(int fd) {
        this.fd = fd;
    }
    @Override
    public void run(){
        PandaHandler.nativeStart(this.fd);
    }
}

public class PandaHandler extends Service {
    String TAG = "PandaHandler";
    ZMQSubHandler subHandler;
    KITTIDataset dataset;
    private Handler mHandler = new Handler();

    private Runnable pandaPeriodicTask = new Runnable() {
        public void run() {
            if (subHandler != null && subHandler.updated("carState")) {
                byte[] carStateData = subHandler.recv("carState");
                if (carStateData != null) {
                    // Process carState data as byte array
                    // Note: This needs to be adapted based on how carState is processed
                    dataset.saveCarStateData(carStateData);
                }
            }

            mHandler.postDelayed(this, 10);
        }
    };

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
                        Log.i(TAG, "Permission granted for serial "+usbDeviceConnection.getSerial());
                        PandaInstance pandaInstance = new PandaInstance(usbDeviceConnection.getFileDescriptor());
                        mHandler.postDelayed(pandaPeriodicTask, 500);
                        new Thread(pandaInstance).start();
                    }
                }
            }
        }
    };

    private static final String ACTION_USB_PERMISSION = "com.example.adas.USB_PERMISSION";

    private void maybeRequestUSBPermission(UsbDevice device, Context context) {
        if (device == null) {
            Log.w(TAG, "maybeRequestUSBPermission got a null device");
            return;
        }
        if (device.getVendorId() == 0xbbaa && device.getProductId() == 0xddcc) {
            PendingIntent pendingIntent = PendingIntent.getBroadcast(context, 0, new Intent(ACTION_USB_PERMISSION), PendingIntent.FLAG_IMMUTABLE);
            ((UsbManager) context.getSystemService(Context.USB_SERVICE)).requestPermission(device, pendingIntent);
        } else {
            Log.w(TAG, "Found a USB device that's not a Panda");
        }
    }

    public static native void nativeStart(int fd);
    public static native void nativeStop();

    @Override
    public void onCreate() {
        super.onCreate();
        System.loadLibrary("pandad");

        dataset = KITTIDataset.getInstance();
        subHandler = new ZMQSubHandler(true, this);
        subHandler.createSubscribers(Arrays.asList("carState"));
    }
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        IntentFilter attachFilter = new IntentFilter();
        attachFilter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        attachFilter.addAction(ACTION_USB_PERMISSION);

        registerReceiver(usbReceiver, attachFilter, Context.RECEIVER_EXPORTED);

        UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);
        HashMap<String, UsbDevice> deviceList = manager.getDeviceList();
        Log.i(TAG, "Number of USB devices found: "+deviceList.size());
        for (UsbDevice usbDevice : deviceList.values())
        {
            maybeRequestUSBPermission(usbDevice, getApplicationContext());
        }

        return START_NOT_STICKY;
    }

    @Override
    public void onDestroy() {
//        super.onDestroy();
        nativeStop();
        unregisterReceiver(usbReceiver);
        if (subHandler != null) {
            subHandler.releaseAll();
        }
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
