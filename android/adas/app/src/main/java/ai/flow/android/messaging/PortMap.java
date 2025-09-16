package messaging;

import android.content.Context;
import android.util.Log;
import org.json.JSONObject;
import org.json.JSONArray;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;

public class PortMap{
    private static final String TAG = "PortMap";
    private static final String CONFIG_FILE = "zmq_topics.json";
    
    public Map<String, Service> services;
    public static final int STARTING_PORT = 5100;
    public static final int RESERVED_PORT = 8022;  // sshd

    public static int newPort(int idx){
        int port = idx + STARTING_PORT;
        return port >= RESERVED_PORT ? port + 1 : port;
    }
    
    public PortMap load(){
        return load(null);
    }
    
    public PortMap load(Context context){
        services = new HashMap<>();

        loadDefaultConfig();
//
//        if (context != null) {
//            // Load from Android assets
//            loadFromAssets(context);
//        } else {
//            // Fallback to default configuration
//            loadDefaultConfig();
//        }
        
        return this;
    }
    
    private void loadFromAssets(Context context) {
        try {
            InputStream is = context.getAssets().open(CONFIG_FILE);
            int size = is.available();
            byte[] buffer = new byte[size];
            is.read(buffer);
            is.close();
            
            String json = new String(buffer, "UTF-8");
            JSONObject config = new JSONObject(json);
            JSONObject zmqConfig = config.getJSONObject("zmq_config");
            
            // Load topic ports from JSON
            JSONObject topicPorts = zmqConfig.getJSONObject("topic_ports");
            JSONArray dataTopics = zmqConfig.getJSONObject("topics").getJSONArray("data_topics");
            
            for (int i = 0; i < dataTopics.length(); i++) {
                String topic = dataTopics.getString(i);
                int port = topicPorts.getInt(topic);
                
                Service service = new Service();
                service.port = port;
                service.expectedFreq = 30.0f; // Default frequency
                service.keepLast = true; // Default to keep last message
                service.log = false; // Default logging
                service.decimation = 1.0f; // No decimation by default
                
                services.put(topic, service);
                Log.d(TAG, "Loaded service: " + topic + " -> port " + port);
            }
            
        } catch (Exception e) {
            Log.e(TAG, "Failed to load configuration from assets", e);
            loadDefaultConfig();
        }
    }
    
    private void loadDefaultConfig() {
        // Default configuration using HashMap for better readability and maintainability
        Map<String, Integer> topicPorts = new HashMap<String, Integer>() {{
            put("wideRoadCameraState", 5555);
            put("wideRoadCameraBuffer", 5556);
            put("gpsLocation", 5557);
            put("gpsData", 5557);
            put("imuData", 5558);
            put("accelerometerData", 5560);
            put("gyroscopeData", 5561);
            put("magnetometerData", 5562);
        }};
        
        for (Map.Entry<String, Integer> entry : topicPorts.entrySet()) {
            Service service = new Service();
            service.port = entry.getValue();
            service.expectedFreq = 30.0f;
            service.keepLast = true;
            service.log = false;
            service.decimation = 1.0f;
            
            services.put(entry.getKey(), service);
        }
        
        Log.d(TAG, "Loaded default configuration with " + topicPorts.size() + " topics");
    }
}
