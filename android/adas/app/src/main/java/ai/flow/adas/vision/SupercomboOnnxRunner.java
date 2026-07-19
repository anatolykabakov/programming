package ai.flow.adas.vision;

import android.content.Context;
import android.graphics.Bitmap;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.nio.FloatBuffer;
import java.util.HashMap;
import java.util.Map;

import ai.onnxruntime.OnnxTensor;
import ai.onnxruntime.OrtEnvironment;
import ai.onnxruntime.OrtLoggingLevel;
import ai.onnxruntime.OrtSession;

/**
 * Minimal openpilot supercombo ONNX runner for Android.
 * Preprocess mirrors openpilot-supercombo-model/openpilot_onnx.py:
 *   resize 512x256 → YUV I420 → 6ch half-res → stack 2 frames → [1,12,128,256]
 *
 * Output parse follows openpilot v0.8.x driving.cc (NOT the buggy demo grouping):
 *   plan @0 (5 MHP × 991), lanes @4955 (4×33×(y,z)), probs @5483, edges @5491.
 * Lateral Y is negated on parse (ONNX Y-right → openpilot Y-left).
 */
public class SupercomboOnnxRunner {
    private static final String TAG = "SupercomboOnnx";

    public static final int MODEL_W = 512;
    public static final int MODEL_H = 256;
    public static final int CH_PER_FRAME = 6;
    public static final int TENSOR_C = 12;
    public static final int TENSOR_H = 128;
    public static final int TENSOR_W = 256;

    // Output layout (supercombo.onnx out=6409), openpilot ~v0.8.x
    private static final int PLAN_END = 4955;
    private static final int LANES_END = PLAN_END + 528;       // 5483
    private static final int LANE_PROB_END = LANES_END + 8;    // 5491
    private static final int ROAD_END = LANE_PROB_END + 264;   // 5755

    private static final int PLAN_MHP_N = 5;
    private static final int PLAN_COLS = 15;
    /** 33×15 means + 33×15 stds + 1 selection logit */
    private static final int PLAN_GROUP = 2 * PLAN_COLS * LaneLines.N + 1; // 991

    private final OrtEnvironment env;
    private final OrtSession session;
    private final float[] desire = new float[8];
    private final float[] traffic = new float[2];
    private final float[] rnnState = new float[512];

    private float[] prevFrame6; // 6*128*256
    private boolean hasPrev;

    private final int[] resizePixels = new int[MODEL_W * MODEL_H];
    private final byte[] yuvI420 = new byte[MODEL_W * MODEL_H * 3 / 2];
    private final float[] currFrame6 = new float[CH_PER_FRAME * TENSOR_H * TENSOR_W];
    private final float[] input12 = new float[TENSOR_C * TENSOR_H * TENSOR_W];

    public interface Listener {
        void onLanes(LaneLines lanes);
        void onError(String msg);
    }

    public SupercomboOnnxRunner(Context context) throws Exception {
        env = OrtEnvironment.getEnvironment();
        File model = resolveModelFile(context);
        Log.i(TAG, "Loading model: " + model.getAbsolutePath() + " (" + model.length() + " bytes)");
        OrtSession.SessionOptions opts = new OrtSession.SessionOptions();
        opts.setIntraOpNumThreads(2);
        try {
            opts.setSessionLogLevel(OrtLoggingLevel.ORT_LOGGING_LEVEL_WARNING);
        } catch (Throwable ignored) {
        }
        try {
            session = env.createSession(model.getAbsolutePath(), opts);
        } catch (Exception e) {
            Log.e(TAG, "createSession failed for " + model.getAbsolutePath()
                    + " size=" + model.length(), e);
            throw e;
        }
        // RHT (right-hand traffic): [1, 0]
        traffic[0] = 1.f;
        traffic[1] = 0.f;
        Log.i(TAG, "ONNX inputs=" + session.getInputNames() + " outputs=" + session.getOutputNames());
    }

    private static File resolveModelFile(Context context) throws Exception {
        File external = new File("/sdcard/adas_models/supercombo.onnx");
        if (external.exists() && external.length() > 1_000_000) {
            return external;
        }
        File cached = new File(context.getFilesDir(), "supercombo.onnx");
        if (cached.exists() && cached.length() > 1_000_000) {
            return cached;
        }
        try (InputStream in = context.getAssets().open("models/supercombo.onnx");
             FileOutputStream out = new FileOutputStream(cached)) {
            byte[] buf = new byte[1 << 20];
            int n;
            while ((n = in.read(buf)) > 0) {
                out.write(buf, 0, n);
            }
            return cached;
        } catch (Exception e) {
            throw new IllegalStateException(
                    "supercombo.onnx not found. Push with:\n" +
                    "  adb shell mkdir -p /sdcard/adas_models\n" +
                    "  adb push openpilot-supercombo-model/supercombo.onnx /sdcard/adas_models/supercombo.onnx",
                    e);
        }
    }

    /** Run on a background thread. Accepts ARGB Bitmap (camera preview size). */
    public synchronized LaneLines run(Bitmap frame, int frameId) throws Exception {
        Bitmap resized = Bitmap.createScaledBitmap(frame, MODEL_W, MODEL_H, true);
        try {
            resized.getPixels(resizePixels, 0, MODEL_W, 0, 0, MODEL_W, MODEL_H);
            rgbToYuvI420(resizePixels, MODEL_W, MODEL_H, yuvI420);
            parseImageYuvI420(yuvI420, MODEL_W, MODEL_H, currFrame6);

            if (!hasPrev) {
                prevFrame6 = currFrame6.clone();
                hasPrev = true;
                return null; // need 2 frames
            }

            // Stack [prev, curr] → [1,12,128,256]
            System.arraycopy(prevFrame6, 0, input12, 0, prevFrame6.length);
            System.arraycopy(currFrame6, 0, input12, prevFrame6.length, currFrame6.length);
            System.arraycopy(currFrame6, 0, prevFrame6, 0, currFrame6.length);

            long[] shape = new long[]{1, TENSOR_C, TENSOR_H, TENSOR_W};
            try (OnnxTensor tImgs = OnnxTensor.createTensor(env, FloatBuffer.wrap(input12), shape);
                 OnnxTensor tDesire = OnnxTensor.createTensor(env, FloatBuffer.wrap(desire), new long[]{1, 8});
                 OnnxTensor tTraffic = OnnxTensor.createTensor(env, FloatBuffer.wrap(traffic), new long[]{1, 2});
                 OnnxTensor tState = OnnxTensor.createTensor(env, FloatBuffer.wrap(rnnState), new long[]{1, 512})) {

                Map<String, OnnxTensor> feeds = new HashMap<>();
                // Bind by canonical names used in this ONNX (see openpilot_onnx.py)
                feeds.put("input_imgs", tImgs);
                feeds.put("desire", tDesire);
                feeds.put("traffic_convention", tTraffic);
                feeds.put("initial_state", tState);

                try (OrtSession.Result result = session.run(feeds)) {
                    Object value = result.get(0).getValue();
                    float[] flat = flattenOutput(value);
                    LaneLines lanes = parseLanes(flat);
                    lanes.frameId = frameId;
                    lanes.timestampMs = ai.flow.adas.TimeUtil.nowMs();

                    // Feed recurrent state from tail of output if present
                    if (flat.length >= ROAD_END + 512) {
                        System.arraycopy(flat, flat.length - 512, rnnState, 0, 512);
                    }
                    return lanes;
                }
            }
        } finally {
            if (resized != frame) {
                resized.recycle();
            }
        }
    }

    private static float[] flattenOutput(Object value) {
        if (value instanceof float[][]) {
            float[][] a = (float[][]) value;
            return a[0];
        }
        if (value instanceof float[]) {
            return (float[]) value;
        }
        throw new IllegalStateException("Unexpected ONNX output type: " + value.getClass());
    }

    private static LaneLines parseLanes(float[] out) {
        LaneLines ll = new LaneLines();
        if (out.length < ROAD_END) {
            Log.w(TAG, "Output too short: " + out.length);
            return ll;
        }

        // --- PLAN: best of 5 MHP ---
        int bestHyp = 0;
        float bestLogit = Float.NEGATIVE_INFINITY;
        for (int i = 0; i < PLAN_MHP_N; i++) {
            float logit = out[(i + 1) * PLAN_GROUP - 1];
            if (logit > bestLogit) {
                bestLogit = logit;
                bestHyp = i;
            }
        }
        int planBase = bestHyp * PLAN_GROUP;
        for (int i = 0; i < LaneLines.N; i++) {
            int row = planBase + i * PLAN_COLS;
            ll.planX[i] = out[row];
            // Model lateral is Y-right for this ONNX; store openpilot Y-left (+left).
            ll.planY[i] = -out[row + 1];
            ll.planZ[i] = out[row + 2];
        }
        ll.planHypIndex = bestHyp;
        ll.hasPlan = true;

        // --- LANES: first 264 = 4×33×(y,z) means; second 264 = stds (ignored) ---
        for (int lane = 0; lane < 4; lane++) {
            int base = PLAN_END + lane * 66;
            for (int i = 0; i < LaneLines.N; i++) {
                ll.lanesY[lane][i] = -out[base + i * 2];
                ll.lanesZ[lane][i] = out[base + i * 2 + 1];
            }
            // official: sigmoid(prob[i*2 + 1])
            ll.laneProbs[lane] = sigmoid(out[LANES_END + lane * 2 + 1]);
        }

        // --- ROAD EDGES: first 132 = 2×33×(y,z) means ---
        for (int edge = 0; edge < 2; edge++) {
            int base = LANE_PROB_END + edge * 66;
            for (int i = 0; i < LaneLines.N; i++) {
                ll.edgesY[edge][i] = -out[base + i * 2];
                ll.edgesZ[edge][i] = out[base + i * 2 + 1];
            }
        }
        return ll;
    }

    private static float sigmoid(float x) {
        return (float) (1.0 / (1.0 + Math.exp(-x)));
    }

    /** RGB888 int pixels → planar YUV I420 (Y full, then U, then V subsampled). */
    static void rgbToYuvI420(int[] argb, int w, int h, byte[] out) {
        int ySize = w * h;
        int uvW = w / 2;
        int uvH = h / 2;
        int uOff = ySize;
        int vOff = ySize + uvW * uvH;
        int yi = 0;
        for (int j = 0; j < h; j++) {
            for (int i = 0; i < w; i++) {
                int c = argb[yi];
                int r = (c >> 16) & 0xff;
                int g = (c >> 8) & 0xff;
                int b = c & 0xff;
                int y = ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
                out[yi] = (byte) clamp(y, 0, 255);
                if ((j % 2 == 0) && (i % 2 == 0)) {
                    int u = ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
                    int v = ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
                    int uvi = (j / 2) * uvW + (i / 2);
                    out[uOff + uvi] = (byte) clamp(u, 0, 255);
                    out[vOff + uvi] = (byte) clamp(v, 0, 255);
                }
                yi++;
            }
        }
    }

    /**
     * Same packing as openpilot_onnx.parse_image / flowpilot YUV420toTensor:
     * 6 planes at H/2 x W/2 from I420 frame of size HxW (here 256x512 → 128x256).
     */
    static void parseImageYuvI420(byte[] frame, int w, int h, float[] out6) {
        int H = (frame.length * 2) / 3 / w; // should equal h
        if (H != h) {
            H = h;
        }
        int hh = H / 2;
        int ww = w / 2;
        int plane = hh * ww;
        // Y subsampled into 4 planes
        for (int j = 0; j < hh; j++) {
            for (int i = 0; i < ww; i++) {
                int y00 = frame[(2 * j) * w + (2 * i)] & 0xff;
                int y10 = frame[(2 * j + 1) * w + (2 * i)] & 0xff;
                int y01 = frame[(2 * j) * w + (2 * i + 1)] & 0xff;
                int y11 = frame[(2 * j + 1) * w + (2 * i + 1)] & 0xff;
                int idx = j * ww + i;
                out6[0 * plane + idx] = y00;
                out6[1 * plane + idx] = y10;
                out6[2 * plane + idx] = y01;
                out6[3 * plane + idx] = y11;
            }
        }
        int uOff = H * w;
        int vOff = uOff + hh * ww;
        for (int i = 0; i < plane; i++) {
            out6[4 * plane + i] = frame[uOff + i] & 0xff;
            out6[5 * plane + i] = frame[vOff + i] & 0xff;
        }
    }

    private static int clamp(int v, int lo, int hi) {
        return Math.max(lo, Math.min(hi, v));
    }

    public void close() {
        try {
            session.close();
        } catch (Exception ignored) {
        }
    }
}
