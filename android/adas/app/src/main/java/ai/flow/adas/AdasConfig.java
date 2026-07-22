package ai.flow.adas;

import android.content.Context;
import android.util.Log;

import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

/**
 * Loads {@code assets/config.json}: node feature flags + camera extrinsic priors.
 */
public final class AdasConfig {
    private static final String TAG = "AdasConfig";
    public static final String ASSET = "config.json";

    public final boolean panda;
    public final boolean zmqBridge;
    public final boolean laneKeep;
    public final boolean localization;
    public final boolean cameraCalib;
    public final boolean visionSupercombo;

    public final String vehicleName;
    public final float wheelbaseM;
    public final float steerRatio;

    /** ISO vehicle: X forward, Y left, Z up (meters). */
    public final float camX;
    public final float camY;
    public final float camZ;
    public final float rollDeg;
    public final float pitchDeg;
    public final float yawDeg;

    public final float fx;
    public final float fy;
    public final float cx;
    public final float cy;
    public final int frameW;
    public final int frameH;

    public final String supercomboAsset;

    /** Native SUB bind / Java PUB connect (sensors → C++). */
    public final String zmqEndpointIn;
    /** Native PUB bind / Java SUB connect (C++ → bag / HUD). */
    public final String zmqEndpointOut;

    private AdasConfig(JSONObject root) throws Exception {
        JSONObject nodes = root.optJSONObject("nodes");
        if (nodes == null) {
            nodes = new JSONObject();
        }
        panda = nodes.optBoolean("panda", true);
        zmqBridge = nodes.optBoolean("zmq_bridge", true);
        laneKeep = nodes.optBoolean("lane_keep", false);
        localization = nodes.optBoolean("localization", false);
        cameraCalib = nodes.optBoolean("camera_calib", false);
        visionSupercombo = nodes.optBoolean("vision_supercombo", true);

        JSONObject vehicle = root.optJSONObject("vehicle");
        if (vehicle == null) {
            vehicle = new JSONObject();
        }
        vehicleName = vehicle.optString("name", "vw_golf_7_mqb");
        wheelbaseM = (float) vehicle.optDouble("wheelbase_m", 2.636);
        steerRatio = (float) vehicle.optDouble("steer_ratio", 15.7);

        JSONObject cam = root.optJSONObject("calibration");
        if (cam != null) {
            cam = cam.optJSONObject("camera");
        }
        if (cam == null) {
            cam = new JSONObject();
        }
        JSONObject pos = cam.optJSONObject("position_m");
        if (pos == null) {
            pos = new JSONObject();
        }
        camX = (float) pos.optDouble("x_forward", 0.0);
        camY = (float) pos.optDouble("y_left", -0.02);
        camZ = (float) pos.optDouble("z_up", 0.7);

        JSONObject rpy = cam.optJSONObject("rpy_deg");
        if (rpy == null) {
            rpy = new JSONObject();
        }
        rollDeg = (float) rpy.optDouble("roll", 0.0);
        pitchDeg = (float) rpy.optDouble("pitch", 0.76);
        yawDeg = (float) rpy.optDouble("yaw", 1.56);

        JSONObject K = cam.optJSONObject("intrinsics_prior");
        if (K == null) {
            K = new JSONObject();
        }
        fx = (float) K.optDouble("fx", 930.0);
        fy = (float) K.optDouble("fy", 930.0);
        cx = (float) K.optDouble("cx", 640.0);
        cy = (float) K.optDouble("cy", 360.0);
        frameW = K.optInt("width", 1280);
        frameH = K.optInt("height", 720);

        supercomboAsset = root.optString("supercombo_asset", "supercombo.onnx");

        JSONObject zmq = root.optJSONObject("zmq");
        if (zmq == null) {
            zmq = new JSONObject();
        }
        zmqEndpointIn = zmq.optString("endpoint_in", "tcp://127.0.0.1:5555");
        zmqEndpointOut = zmq.optString("endpoint_out", "tcp://127.0.0.1:5556");
    }

    public static AdasConfig loadDefaults() {
        try {
            return new AdasConfig(new JSONObject());
        } catch (Exception impossible) {
            throw new RuntimeException(impossible);
        }
    }

    public static AdasConfig load(Context context) {
        try (InputStream in = context.getAssets().open(ASSET);
             BufferedReader br = new BufferedReader(new InputStreamReader(in, StandardCharsets.UTF_8))) {
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = br.readLine()) != null) {
                sb.append(line).append('\n');
            }
            AdasConfig cfg = new AdasConfig(new JSONObject(sb.toString()));
            Log.i(TAG, "Loaded " + ASSET
                    + " vision=" + cfg.visionSupercombo
                    + " panda=" + cfg.panda
                    + " lane_keep=" + cfg.laneKeep
                    + " localization=" + cfg.localization
                    + " cam h=" + cfg.camZ
                    + " pitch=" + cfg.pitchDeg
                    + " zmq_in=" + cfg.zmqEndpointIn
                    + " zmq_out=" + cfg.zmqEndpointOut);
            return cfg;
        } catch (Exception e) {
            Log.w(TAG, "Failed to load " + ASSET + ", using defaults", e);
            return loadDefaults();
        }
    }
}
