package ai.flow.adas.vision;

/**
 * Parsed supercombo outputs in openpilot ego / calibrated frame:
 * X forward (m), Y left (m), Z up (m).
 *
 * This ONNX emits lateral Y as right-positive; {@link SupercomboOnnxRunner}
 * negates Y on parse so lanes/plan/edges here use openpilot Y-left.
 *
 * Layout matches openpilot ~v0.8.x driving.cc (out=6409):
 *   [0:4955)     PLAN — 5 MHP trajectories
 *   [4955:5483)  LANES — 4×33×(y,z) means then stds
 *   [5483:5491)  lane probs
 *   [5491:5755)  ROAD EDGES — 2×33×(y,z) means then stds
 */
public class LaneLines {
    public static final int N = 33;

    /** Longitudinal sample points (meters) for lanes/edges. */
    public static final float[] X_IDXS = new float[]{
            0.f, 0.1875f, 0.75f, 1.6875f, 3.f, 4.6875f,
            6.75f, 9.1875f, 12.f, 15.1875f, 18.75f, 22.6875f,
            27.f, 31.6875f, 36.75f, 42.1875f, 48.f, 54.1875f,
            60.75f, 67.6875f, 75.f, 82.6875f, 90.75f, 99.1875f,
            108.f, 117.1875f, 126.75f, 136.6875f, 147.f, 157.6875f,
            168.75f, 180.1875f, 192.f
    };

    /** 4 lanes: leftFar, leftNear, rightNear, rightFar — lateral y at X_IDXS. */
    public final float[][] lanesY = new float[4][N];
    /** Optional lane height z (often unused for ground overlay). */
    public final float[][] lanesZ = new float[4][N];
    /** 2 road edges: left, right. */
    public final float[][] edgesY = new float[2][N];
    public final float[][] edgesZ = new float[2][N];
    public final float[] laneProbs = new float[4];

    /** Best PLAN hypothesis (primary driving output), length N. */
    public final float[] planX = new float[N];
    public final float[] planY = new float[N];
    public final float[] planZ = new float[N];
    public int planHypIndex = -1;
    public boolean hasPlan;

    public long timestampMs;
    public int frameId;

    public LaneLines copy() {
        LaneLines o = new LaneLines();
        for (int i = 0; i < 4; i++) {
            System.arraycopy(lanesY[i], 0, o.lanesY[i], 0, N);
            System.arraycopy(lanesZ[i], 0, o.lanesZ[i], 0, N);
            o.laneProbs[i] = laneProbs[i];
        }
        for (int i = 0; i < 2; i++) {
            System.arraycopy(edgesY[i], 0, o.edgesY[i], 0, N);
            System.arraycopy(edgesZ[i], 0, o.edgesZ[i], 0, N);
        }
        System.arraycopy(planX, 0, o.planX, 0, N);
        System.arraycopy(planY, 0, o.planY, 0, N);
        System.arraycopy(planZ, 0, o.planZ, 0, N);
        o.planHypIndex = planHypIndex;
        o.hasPlan = hasPlan;
        o.timestampMs = timestampMs;
        o.frameId = frameId;
        return o;
    }
}
