package ai.flow.android;

import android.content.Context;
import android.util.Log;
import org.json.JSONObject;
import org.json.JSONArray;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class ZMQConfigLoader {
    private static final String TAG = "ZMQConfigLoader";
    private static final String CONFIG_FILE = "zmq_topics.json";
    
    private static ZMQConfigLoader instance;
    private static JSONObject config;
    
    private ZMQConfigLoader(Context context) {
        loadConfig(context);
    }
    
    public static synchronized ZMQConfigLoader getInstance(Context context) {
        if (instance == null) {
            instance = new ZMQConfigLoader(context);
        }
        return instance;
    }
    
    private void loadConfig(Context context) {
        try {
            InputStream is = context.getAssets().open(CONFIG_FILE);
            int size = is.available();
            byte[] buffer = new byte[size];
            is.read(buffer);
            is.close();
            
            String json = new String(buffer, "UTF-8");
            config = new JSONObject(json);
            Log.i(TAG, "ZMQ configuration loaded successfully");
            
        } catch (IOException e) {
            Log.e(TAG, "Failed to load ZMQ configuration", e);
            // Fallback to default config
            createDefaultConfig();
        } catch (Exception e) {
            Log.e(TAG, "Error parsing ZMQ configuration", e);
            createDefaultConfig();
        }
    }
    
    private void createDefaultConfig() {
        try {
            config = new JSONObject();
            JSONObject zmqConfig = new JSONObject();
            
            // Host
            zmqConfig.put("host", "127.0.0.1");
            
            // Ports
            JSONObject ports = new JSONObject();
            ports.put("camera", 5555);
            ports.put("gps", 5557);
            ports.put("imu", 5558);
            ports.put("counter_start", 5556);
            ports.put("counter_end", 5560);
            zmqConfig.put("ports", ports);
            
            // Topics
            JSONObject topics = new JSONObject();
            JSONArray dataTopics = new JSONArray();
            dataTopics.put("wideRoadCameraState");
            dataTopics.put("wideRoadCameraBuffer");
            dataTopics.put("gpsLocation");
            dataTopics.put("gpsData");
            dataTopics.put("imuData");
            dataTopics.put("accelerometerData");
            dataTopics.put("gyroscopeData");
            dataTopics.put("magnetometerData");
            topics.put("data_topics", dataTopics);
            topics.put("counter_topic", "messageCounters");
            zmqConfig.put("topics", topics);
            
            // Topic ports
            JSONObject topicPorts = new JSONObject();
            topicPorts.put("wideRoadCameraState", 5555);
            topicPorts.put("wideRoadCameraBuffer", 5555);
            topicPorts.put("gpsLocation", 5557);
            topicPorts.put("gpsData", 5557);
            topicPorts.put("imuData", 5558);
            topicPorts.put("accelerometerData", 5558);
            topicPorts.put("gyroscopeData", 5558);
            topicPorts.put("magnetometerData", 5558);
            zmqConfig.put("topic_ports", topicPorts);
            
            config.put("zmq_config", zmqConfig);
            Log.i(TAG, "Created default ZMQ configuration");
            
        } catch (Exception e) {
            Log.e(TAG, "Failed to create default configuration", e);
        }
    }
    
    // Getters
    public String getHost() {
        try {
            return config.getJSONObject("zmq_config").getString("host");
        } catch (Exception e) {
            Log.e(TAG, "Error getting host", e);
            return "127.0.0.1";
        }
    }
    
    public int getCameraPort() {
        try {
            return config.getJSONObject("zmq_config").getJSONObject("ports").getInt("camera");
        } catch (Exception e) {
            Log.e(TAG, "Error getting camera port", e);
            return 5555;
        }
    }
    
    public int getGPSPort() {
        try {
            return config.getJSONObject("zmq_config").getJSONObject("ports").getInt("gps");
        } catch (Exception e) {
            Log.e(TAG, "Error getting GPS port", e);
            return 5557;
        }
    }
    
    public int getIMUPort() {
        try {
            return config.getJSONObject("zmq_config").getJSONObject("ports").getInt("imu");
        } catch (Exception e) {
            Log.e(TAG, "Error getting IMU port", e);
            return 5558;
        }
    }
    
    public int getCounterPortStart() {
        try {
            return config.getJSONObject("zmq_config").getJSONObject("ports").getInt("counter_start");
        } catch (Exception e) {
            Log.e(TAG, "Error getting counter start port", e);
            return 5556;
        }
    }
    
    public int getCounterPortEnd() {
        try {
            return config.getJSONObject("zmq_config").getJSONObject("ports").getInt("counter_end");
        } catch (Exception e) {
            Log.e(TAG, "Error getting counter end port", e);
            return 5560;
        }
    }
    
    public String[] getDataTopics() {
        try {
            JSONArray topicsArray = config.getJSONObject("zmq_config").getJSONObject("topics").getJSONArray("data_topics");
            String[] topics = new String[topicsArray.length()];
            for (int i = 0; i < topicsArray.length(); i++) {
                topics[i] = topicsArray.getString(i);
            }
            return topics;
        } catch (Exception e) {
            Log.e(TAG, "Error getting data topics", e);
            return new String[]{
                "wideRoadCameraState", "wideRoadCameraBuffer", 
                "gpsLocation", "gpsData", "imuData", 
                "accelerometerData", "gyroscopeData", "magnetometerData"
            };
        }
    }
    
    public String getCounterTopic() {
        try {
            return config.getJSONObject("zmq_config").getJSONObject("topics").getString("counter_topic");
        } catch (Exception e) {
            Log.e(TAG, "Error getting counter topic", e);
            return "messageCounters";
        }
    }
    
    public int getPortForTopic(String topic) {
        try {
            return config.getJSONObject("zmq_config").getJSONObject("topic_ports").getInt(topic);
        } catch (Exception e) {
            Log.e(TAG, "Error getting port for topic: " + topic, e);
            return -1;
        }
    }
    
    public String getEndpointForTopic(String topic) {
        int port = getPortForTopic(topic);
        if (port == -1) {
            return null;
        }
        return "tcp://" + getHost() + ":" + port;
    }
    
    public String getCameraEndpoint() {
        return "tcp://" + getHost() + ":" + getCameraPort();
    }
    
    public String getGPSEndpoint() {
        return "tcp://" + getHost() + ":" + getGPSPort();
    }
    
    public String getIMUEndpoint() {
        return "tcp://" + getHost() + ":" + getIMUPort();
    }
    
    public String getCounterEndpoint(int port) {
        return "tcp://" + getHost() + ":" + port;
    }
}

