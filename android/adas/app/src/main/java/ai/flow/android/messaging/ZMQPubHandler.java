package messaging;
//zmq

import android.content.Context;
import android.util.Log;
import org.zeromq.ZMQ;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ZMQPubHandler{
    private static final String TAG = "ZMQPubHandler";
    
    private ZMQ.Context context;
    private PortMap portMap;
    public Map<String, ZMQ.Socket> sockets = new HashMap<>();

    public ZMQPubHandler(){
        this(null);
    }
    
    public ZMQPubHandler(Context androidContext){
        this.context = Factory.getContext();
        this.portMap = Factory.getPortmap(androidContext);
        if (this.portMap != null) {
            this.portMap.load(androidContext);
        }
    }

    public boolean createPublishers(List<String> topicList){
        boolean status = true;
        for (String topic : topicList){
            status = status & createPublisher(topic);
        }
        return status;
    }

    public boolean createPublisher(String topic){
        try {
            if (portMap == null || portMap.services == null) {
                Log.e(TAG, "PortMap not initialized");
                return false;
            }
            
            Service service = portMap.services.get(topic);
            if (service == null) {
                Log.e(TAG, "Service not found for topic: " + topic);
                return false;
            }
            
            ZMQ.Socket pub;
            int port = service.port;
            pub = context.socket(ZMQ.PUB);
            pub.bind(Utils.getSocketPath(Integer.toString(port)));
            this.sockets.put(topic, pub);
            Log.d(TAG, "Publisher created: " + topic + " on port " + port);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "Failed to create publisher for topic: " + topic, e);
            return false;
        }
    }

    public void releasePublishers(List<String> topicList){
        for (String topic : topicList){
            this.sockets.get(topic).close();
        }
    }

    public void releaseAll(){
        for(String topic : this.sockets.keySet()) {
            this.sockets.get(topic).close();
        }
    }

    public void publish(Map<String, byte[]> data){
        for(String topic : data.keySet()) {
            this.sockets.get(topic).send(data.get(topic), 0);
        }
    }

    public void publishBuffer(String topic, ByteBuffer data){
        data.rewind();
        ZMQ.Socket socket = this.sockets.get(topic);
        if (socket != null) {
            socket.sendByteBuffer(data, 0);
        } else {
            Log.e(TAG, "Socket not found for topic: " + topic);
        }
    }

    public void publish(String topic, byte[] data){
        ZMQ.Socket socket = this.sockets.get(topic);
        if (socket != null) {
            socket.send(data, 0);
        } else {
            Log.e(TAG, "Socket not found for topic: " + topic);
        }
    }
}
