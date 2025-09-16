package messaging;
//zmq

import android.content.Context;
import android.util.Log;
import org.zeromq.ZMQ;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ZMQSubHandler{
    private static final String TAG = "ZMQSubHandler";
    
    private ZMQ.Context context;
    private PortMap portMap;
    public int subCount = 0;
    public Map<String, Integer> pollTopics = new HashMap<>();
    public ZMQ.Poller poller;
    public Map<String, ZMQ.Socket> sockets = new HashMap<>();
    public final boolean conflate;

    public ZMQSubHandler(boolean conflate){
        this(conflate, null);
    }
    
    public ZMQSubHandler(boolean conflate, Context androidContext){
        this.conflate = conflate;
        this.context = Factory.getContext();
        this.portMap = Factory.getPortmap(androidContext);
        this.poller = context.poller();
    }

    public boolean createSubscribers(List<String> topicList){
        boolean status = true;
        for (String topic : topicList){
            status = status & createSubscriber(topic);
        }
        return status;
    }

    public boolean createSubscriber(String topic){
        try {
            ZMQ.Socket socket;
            int port = portMap.services.get(topic).port;
            socket = context.socket(ZMQ.SUB);
            socket.setConflate(portMap.services.get(topic).keepLast);
            socket.connect(Utils.getSocketPath(Integer.toString(port)));
            socket.subscribe("".getBytes());
            poller.register(socket, ZMQ.Poller.POLLIN);
            this.sockets.put(topic, socket);
            this.pollTopics.put(topic, subCount);
            Log.d(TAG, "Subscriber created: " + topic + " on port " + port);
            subCount++;
            return true;
        } catch (Exception e) {
            Log.e(TAG, "Failed to create subscriber for topic: " + topic, e);
            return false;
        }
    }

    public void releaseSubscribers(List<String> topicList){
        for (String topic : topicList){
            this.sockets.get(topic).close();
        }
    }

    public void releaseAll(){
        for(String topic : this.sockets.keySet()) {
            this.sockets.get(topic).close();
        }
    }

    public byte[] recvMultipart(String topic){
        ZMQ.Socket socket;
        socket = this.sockets.get(topic);
        socket.recv(); // receive topic
        return socket.recv(); // actual data
    }

    public ByteBuffer recvBuffer(String topic){
        ZMQ.Socket socket = this.sockets.get(topic);
        return ByteBuffer.wrap(socket.recv());
    }

    public byte[] recv(String topic){
        try {
            return recvBuffer(topic).array();
        } catch (Exception e) {
            Log.e(TAG, "Failed to receive data for topic: " + topic, e);
            return null;
        }
    }

    public boolean updated(String topic){
        poller.poll(0);
        return poller.pollin(pollTopics.get(topic));
    }

    public Map<String, Boolean> updated(List<String> topicList){
        poller.poll(0);
        Map<String, Boolean> status = new HashMap<>();
        for (String topic : topicList){
            status.put(topic, poller.pollin(pollTopics.get(topic)));
        }
        return status;
    }

    public Map<String, byte[]> getData(List<String> topicList){
        Map<String, byte[]> allData = new HashMap<>();
        byte[] data;
        for (String topic : topicList){
            data = getData(topic);
            allData.put(topic, data);
        }
        return allData;
    }

    public byte[] getData(String topic){
        ZMQ.Socket socket = this.sockets.get(topic);
        return socket.recv();
    }
}
