package ai.flow.adas.vision;

public final class CameraOdometry {

    public static final int POSE_SIZE = 12;
    public static final int TEMPORAL_SIZE = 512;
    public static final int OUTPUT_SIZE = 6409;
    public static final int POSE_IDX = OUTPUT_SIZE - TEMPORAL_SIZE - POSE_SIZE;

    public final float[] trans = new float[3];
    public final float[] rot = new float[3];
    public final float[] transStd = new float[3];
    public final float[] rotStd = new float[3];
    public boolean valid;

    public static CameraOdometry parse(float[] out) {
        CameraOdometry o = new CameraOdometry();
        if (out == null || out.length < POSE_IDX + POSE_SIZE) {
            return o;
        }
        o.trans[0] = out[POSE_IDX];
        o.trans[1] = out[POSE_IDX + 1];
        o.trans[2] = out[POSE_IDX + 2];
        o.rot[0] = (float) Math.toRadians(out[POSE_IDX + 3]);
        o.rot[1] = (float) Math.toRadians(out[POSE_IDX + 4]);
        o.rot[2] = (float) Math.toRadians(out[POSE_IDX + 5]);
        for (int i = 0; i < 3; i++) {
            o.transStd[i] = (float) Math.exp(out[POSE_IDX + 6 + i]);
            o.rotStd[i] = (float) (Math.exp(out[POSE_IDX + 9 + i]) * Math.PI / 180.0);
        }
        o.valid = Float.isFinite(o.trans[0]) && Float.isFinite(o.trans[1]) && Float.isFinite(o.trans[2]);
        return o;
    }
}
