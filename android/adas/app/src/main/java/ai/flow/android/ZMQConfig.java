package ai.flow.android;

import android.content.Context;

public class ZMQConfig {
    private static final String TAG = "ZMQConfig";
    private static ZMQConfigLoader loader;
    
    // Initialize with context
    public static void init(Context context) {
        if (loader == null) {
            loader = ZMQConfigLoader.getInstance(context);
        }
    }
    
    // Utility methods that delegate to ZMQConfigLoader
    public static String getHost() {
        return loader != null ? loader.getHost() : "127.0.0.1";
    }
    
    public static int getCameraPort() {
        return loader != null ? loader.getCameraPort() : 5555;
    }
    
    public static int getGPSPort() {
        return loader != null ? loader.getGPSPort() : 5557;
    }
    
    public static int getIMUPort() {
        return loader != null ? loader.getIMUPort() : 5558;
    }
    
    public static int getCounterPortStart() {
        return loader != null ? loader.getCounterPortStart() : 5556;
    }
    
    public static int getCounterPortEnd() {
        return loader != null ? loader.getCounterPortEnd() : 5560;
    }
    
    public static String[] getDataTopics() {
        return loader != null ? loader.getDataTopics() : new String[]{
            "wideRoadCameraState", "wideRoadCameraBuffer", 
            "gpsLocation", "gpsData", "imuData", 
            "accelerometerData", "gyroscopeData", "magnetometerData"
        };
    }
    
    public static String getCounterTopic() {
        return loader != null ? loader.getCounterTopic() : "messageCounters";
    }
    
    public static int getPortForTopic(String topic) {
        return loader != null ? loader.getPortForTopic(topic) : -1;
    }
    
    public static String getEndpointForTopic(String topic) {
        return loader != null ? loader.getEndpointForTopic(topic) : null;
    }
    
    public static String getCameraEndpoint() {
        return loader != null ? loader.getCameraEndpoint() : "tcp://127.0.0.1:5555";
    }
    
    public static String getGPSEndpoint() {
        return loader != null ? loader.getGPSEndpoint() : "tcp://127.0.0.1:5557";
    }
    
    public static String getIMUEndpoint() {
        return loader != null ? loader.getIMUEndpoint() : "tcp://127.0.0.1:5558";
    }
    
    public static String getCounterEndpoint(int port) {
        return loader != null ? loader.getCounterEndpoint(port) : "tcp://127.0.0.1:" + port;
    }
}
