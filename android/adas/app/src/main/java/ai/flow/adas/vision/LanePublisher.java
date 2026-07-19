package ai.flow.adas.vision;

import android.util.Log;

import org.json.JSONArray;
import org.json.JSONObject;
import org.zeromq.SocketType;
import org.zeromq.ZContext;
import org.zeromq.ZMQ;

/**
 * Publishes lane lines for a C++ process over ZMQ PUB.
 * Default endpoint: tcp://127.0.0.1:5599  topic "lanes"
 *
 * C++ (cppzmq) example:
 *   zmq::socket_t sub(ctx, ZMQ_SUB);
 *   sub.connect("tcp://127.0.0.1:5599");
 *   sub.set(zmq::sockopt::subscribe, "lanes");
 *   // recv multipart: [topic][json]
 */
public class LanePublisher {
    private static final String TAG = "LanePublisher";

    private final ZContext zctx = new ZContext();
    private final ZMQ.Socket pub;
    private final String topic;

    public LanePublisher() {
        this("tcp://*:5599", "lanes");
    }

    public LanePublisher(String bindEndpoint, String topic) {
        this.topic = topic;
        pub = zctx.createSocket(SocketType.PUB);
        pub.bind(bindEndpoint);
        Log.i(TAG, "Publishing lanes on " + bindEndpoint + " topic=" + topic);
    }

    public synchronized void publish(LaneLines ll) {
        if (ll == null) {
            return;
        }
        try {
            JSONObject o = new JSONObject();
            o.put("t", ll.timestampMs);
            o.put("frameId", ll.frameId);
            o.put("x", toJson(LaneLines.X_IDXS));
            JSONArray lanes = new JSONArray();
            for (int i = 0; i < 4; i++) {
                JSONObject lane = new JSONObject();
                lane.put("y", toJson(ll.lanesY[i]));
                lane.put("prob", ll.laneProbs[i]);
                lanes.put(lane);
            }
            o.put("lanes", lanes);
            JSONArray edges = new JSONArray();
            for (int i = 0; i < 2; i++) {
                edges.put(new JSONObject().put("y", toJson(ll.edgesY[i])));
            }
            o.put("edges", edges);
            if (ll.hasPlan) {
                o.put("planX", toJson(ll.planX));
                o.put("planY", toJson(ll.planY));
                o.put("planZ", toJson(ll.planZ));
                o.put("planHyp", ll.planHypIndex);
            }

            byte[] body = o.toString().getBytes(ZMQ.CHARSET);
            pub.sendMore(topic);
            pub.send(body);
        } catch (Exception e) {
            Log.e(TAG, "publish failed", e);
        }
    }

    private static JSONArray toJson(float[] a) throws org.json.JSONException {
        JSONArray arr = new JSONArray();
        for (float v : a) {
            arr.put((double) v);
        }
        return arr;
    }

    public void close() {
        try {
            pub.close();
            zctx.close();
        } catch (Exception ignored) {
        }
    }
}
