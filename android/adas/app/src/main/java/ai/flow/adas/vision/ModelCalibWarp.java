package ai.flow.adas.vision;

import android.graphics.Bitmap;

/**
 * Flowpilot-style model warp: camera K + calib RPY → 3×3 homography that maps
 * model-frame pixels → camera pixels (same convention as TransformCL / OpenCL).
 *
 * {@code warp = K · view_from_device · R(rpy) · inv(medmodel_K · view_from_device)}
 */
public final class ModelCalibWarp {
    public static final int MODEL_W = 512;
    public static final int MODEL_H = 256;

    /** openpilot / flowpilot medmodel intrinsics. */
    private static final float MED_FL = 910.0f;
    private static final float MED_CY = 47.6f;

    /** view_from_device: device (x forward, y right, z down) → camera view. */
    private static final float[] VIEW_FROM_DEVICE = {
            0, 1, 0,
            0, 0, 1,
            1, 0, 0
    };

    private ModelCalibWarp() {}

    /**
     * @param rollRad  calib roll (rad)
     * @param pitchRad calib pitch (rad)
     * @param yawRad   calib yaw (rad)
     * @param fx,fy,cx,cy camera intrinsics for the source bitmap size
     * @return row-major 3×3, model → camera
     */
    public static float[] warpMatrix(double rollRad, double pitchRad, double yawRad,
                                     float fx, float fy, float cx, float cy) {
        float[] K = {
                fx, 0, cx,
                0, fy, cy,
                0, 0, 1
        };
        float[] medK = {
                MED_FL, 0, 0.5f * MODEL_W,
                0, MED_FL, MED_CY,
                0, 0, 1
        };
        float[] medFromCalib = mul3(medK, VIEW_FROM_DEVICE);
        float[] calibFromModel = inv3(medFromCalib);
        float[] deviceFromCalib = rotFromEuler(rollRad, pitchRad, yawRad);
        float[] viewFromCalib = mul3(VIEW_FROM_DEVICE, deviceFromCalib);
        float[] cameraFromCalib = mul3(K, viewFromCalib);
        return mul3(cameraFromCalib, calibFromModel);
    }

    /** Same as {@link #warpMatrix} with degrees. */
    public static float[] warpMatrixDeg(float rollDeg, float pitchDeg, float yawDeg,
                                        float fx, float fy, float cx, float cy) {
        return warpMatrix(
                Math.toRadians(rollDeg),
                Math.toRadians(pitchDeg),
                Math.toRadians(yawDeg),
                fx, fy, cx, cy);
    }

    /**
     * Warp camera ARGB bitmap into model-sized ARGB using M (model→camera).
     * Out-of-bounds samples are black.
     */
    public static Bitmap warpToModel(Bitmap src, float[] mModelToCam) {
        final int sw = src.getWidth();
        final int sh = src.getHeight();
        int[] srcPx = new int[sw * sh];
        src.getPixels(srcPx, 0, sw, 0, 0, sw, sh);

        int[] dstPx = new int[MODEL_W * MODEL_H];
        final float m00 = mModelToCam[0], m01 = mModelToCam[1], m02 = mModelToCam[2];
        final float m10 = mModelToCam[3], m11 = mModelToCam[4], m12 = mModelToCam[5];
        final float m20 = mModelToCam[6], m21 = mModelToCam[7], m22 = mModelToCam[8];

        for (int y = 0; y < MODEL_H; y++) {
            for (int x = 0; x < MODEL_W; x++) {
                float X = m00 * x + m01 * y + m02;
                float Y = m10 * x + m11 * y + m12;
                float W = m20 * x + m21 * y + m22;
                if (Math.abs(W) < 1e-8f) {
                    dstPx[y * MODEL_W + x] = 0xff000000;
                    continue;
                }
                float sx = X / W;
                float sy = Y / W;
                dstPx[y * MODEL_W + x] = sampleBilinear(srcPx, sw, sh, sx, sy);
            }
        }
        Bitmap out = Bitmap.createBitmap(MODEL_W, MODEL_H, Bitmap.Config.ARGB_8888);
        out.setPixels(dstPx, 0, MODEL_W, 0, 0, MODEL_W, MODEL_H);
        return out;
    }

    private static int sampleBilinear(int[] px, int w, int h, float sx, float sy) {
        if (sx < 0 || sy < 0 || sx >= w - 1 || sy >= h - 1) {
            if (sx < -0.5f || sy < -0.5f || sx >= w - 0.5f || sy >= h - 0.5f) {
                return 0xff000000;
            }
            int ix = clamp((int) Math.floor(sx + 0.5f), 0, w - 1);
            int iy = clamp((int) Math.floor(sy + 0.5f), 0, h - 1);
            return px[iy * w + ix];
        }
        int x0 = (int) Math.floor(sx);
        int y0 = (int) Math.floor(sy);
        float fx = sx - x0;
        float fy = sy - y0;
        int x1 = x0 + 1;
        int y1 = y0 + 1;
        int c00 = px[y0 * w + x0];
        int c10 = px[y0 * w + x1];
        int c01 = px[y1 * w + x0];
        int c11 = px[y1 * w + x1];
        return lerpArgb(lerpArgb(c00, c10, fx), lerpArgb(c01, c11, fx), fy);
    }

    private static int lerpArgb(int a, int b, float t) {
        int ar = (a >> 16) & 0xff, ag = (a >> 8) & 0xff, ab = a & 0xff;
        int br = (b >> 16) & 0xff, bg = (b >> 8) & 0xff, bb = b & 0xff;
        int r = (int) (ar + (br - ar) * t + 0.5f);
        int g = (int) (ag + (bg - ag) * t + 0.5f);
        int bl = (int) (ab + (bb - ab) * t + 0.5f);
        return 0xff000000 | (r << 16) | (g << 8) | bl;
    }

    /** Matches flowpilot Preprocess.eulerAnglesToRotationMatrix(..., isDegrees=false). */
    static float[] rotFromEuler(double roll, double pitch, double yaw) {
        float cp = (float) Math.cos(pitch);
        float sp = (float) Math.sin(pitch);
        float sr = (float) Math.sin(roll);
        float cr = (float) Math.cos(roll);
        float sy = (float) Math.sin(yaw);
        float cy = (float) Math.cos(yaw);

        // Same 3×3 as Preprocess before Nd4j transpose (row-major).
        float[] rot = {
                cp * cy, cp * sy, -sp,
                (sr * sp * cy) - (cr * sy), (sr * sp * sy) + (cr * cy), sr * cp,
                (cr * sp * cy) + (sr * sy), (cr * sp * sy) - (sr * cy), cr * cp
        };
        return transpose3(rot);
    }

    static float[] mul3(float[] a, float[] b) {
        float[] r = new float[9];
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                r[i * 3 + j] =
                        a[i * 3] * b[j] + a[i * 3 + 1] * b[3 + j] + a[i * 3 + 2] * b[6 + j];
            }
        }
        return r;
    }

    static float[] transpose3(float[] m) {
        return new float[]{
                m[0], m[3], m[6],
                m[1], m[4], m[7],
                m[2], m[5], m[8]
        };
    }

    static float[] inv3(float[] m) {
        float a = m[0], b = m[1], c = m[2];
        float d = m[3], e = m[4], f = m[5];
        float g = m[6], h = m[7], i = m[8];
        float A = e * i - f * h;
        float B = f * g - d * i;
        float C = d * h - e * g;
        float det = a * A + b * B + c * C;
        if (Math.abs(det) < 1e-12f) {
            throw new IllegalArgumentException("singular 3x3");
        }
        float invDet = 1.0f / det;
        return new float[]{
                A * invDet, (c * h - b * i) * invDet, (b * f - c * e) * invDet,
                B * invDet, (a * i - c * g) * invDet, (c * d - a * f) * invDet,
                C * invDet, (b * g - a * h) * invDet, (a * e - b * d) * invDet
        };
    }

    private static int clamp(int v, int lo, int hi) {
        return Math.max(lo, Math.min(hi, v));
    }
}
