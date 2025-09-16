package ai.flow.android;

import android.util.Log;

public class AdasAppHandler {
    private static final String TAG = "AdasAppHandler";
    
    // Load native library
    static {
        try {
            System.loadLibrary("adas_app");
            Log.i(TAG, "Native library zmq_reader loaded successfully");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load native library zmq_reader", e);
        }
    }
    
    // Native methods
    public static native void nativeStart();
    public static native void nativeStop();
    
    // Static methods for easy access
    public static void startStatic() {
        Log.i(TAG, "Starting ZMQReader via native code...");
        try {
            nativeStart();
            Log.i(TAG, "ZMQReader started successfully");
        } catch (Exception e) {
            Log.e(TAG, "Error starting ZMQReader", e);
        }
    }
    
    public static void stopStatic() {
        Log.i(TAG, "Stopping ZMQReader via native code...");
        try {
            nativeStop();
            Log.i(TAG, "ZMQReader stopped successfully");
        } catch (Exception e) {
            Log.e(TAG, "Error stopping ZMQReader", e);
        }
    }
}