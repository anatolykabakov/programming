package ai.flow.adas.vision;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Typeface;
import android.util.AttributeSet;
import android.view.View;

public class LaneOverlayView extends View {
    private static final float MIN_LANE_PROB = 0.3f;
    private static final float MAX_TORQUE_CNM = 300f;

    private final Paint lanePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint edgePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint pathPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint ppArcPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint ppLdPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint ppTargetPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint ppRayPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint hudPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint hudFillPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint textBgPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path path = new Path();
    private final Matrix drawMatrix = new Matrix();
    private final float[] mapPt = new float[2];

    private volatile LaneLines lanes;

    private float fx = 930f;
    private float fy = 930f;
    private float cx = 640f;
    private float cy = 360f;
    private float cameraHeight = 1.22f;
    private float frameW = 1280f;
    private float frameH = 720f;
    private float waypointShift = 1.40f;
    private float steerRatio = 15.7f;
    private float rollDeg = 0f;
    private float pitchDeg = 0f;
    private float yawDeg = 0f;

    /** flowpilot OnRoadScreen path lift (m) on remapped camera-up axis. */
    private static final float PATH_LIFT_M = 1.28f;

    /**
     * Camera-frame Rt = V · R(rpy) · V⁻¹ with the same R as {@link ModelCalibWarp}
     * (not LibGDX setFromEulerAnglesRad — that swaps pitch/yaw vs the warp).
     */
    private float r00 = 1, r01 = 0, r02 = 0;
    private float r10 = 0, r11 = 1, r12 = 0;
    private float r20 = 0, r21 = 0, r22 = 1;


    private volatile boolean ppValid = false;
    private volatile boolean ppHasTarget = false;
    private volatile float ppTargetX;
    private volatile float ppTargetY;
    private volatile float ppLookaheadM;
    private volatile float ppCurvature;
    private volatile float ppSteerRad;
    private volatile String ppStatus = "";


    private volatile boolean steerValid = false;
    private volatile String hcaStatus = "";
    private volatile boolean hcaValid = false;
    private volatile int torqueCnm;
    private volatile boolean steerEnabled;

    public LaneOverlayView(Context context) {
        super(context);
        init();
    }

    public LaneOverlayView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        lanePaint.setStyle(Paint.Style.STROKE);
        lanePaint.setStrokeWidth(6f);
        lanePaint.setColor(Color.YELLOW);
        edgePaint.setStyle(Paint.Style.STROKE);
        edgePaint.setStrokeWidth(4f);
        edgePaint.setColor(Color.RED);
        pathPaint.setStyle(Paint.Style.STROKE);
        pathPaint.setStrokeWidth(8f);
        pathPaint.setColor(Color.GREEN);

        ppArcPaint.setStyle(Paint.Style.STROKE);
        ppArcPaint.setStrokeWidth(7f);
        ppArcPaint.setColor(Color.MAGENTA);
        ppLdPaint.setStyle(Paint.Style.STROKE);
        ppLdPaint.setStrokeWidth(2.5f);
        ppLdPaint.setColor(Color.rgb(255, 128, 0));
        ppTargetPaint.setStyle(Paint.Style.FILL);
        ppTargetPaint.setColor(Color.CYAN);
        ppRayPaint.setStyle(Paint.Style.STROKE);
        ppRayPaint.setStrokeWidth(3f);
        ppRayPaint.setColor(Color.CYAN);

        hudPaint.setStyle(Paint.Style.STROKE);
        hudPaint.setStrokeWidth(3f);
        hudPaint.setColor(Color.rgb(220, 220, 220));
        hudFillPaint.setStyle(Paint.Style.FILL);
        hudFillPaint.setColor(Color.argb(140, 30, 30, 30));

        textPaint.setColor(Color.rgb(255, 200, 0));
        textPaint.setTextSize(28f);
        textPaint.setTypeface(Typeface.MONOSPACE);
        textBgPaint.setColor(Color.argb(120, 0, 0, 0));
        textBgPaint.setStyle(Paint.Style.FILL);

        setWillNotDraw(false);
    }

    public void setIntrinsics(float fx, float fy, float cx, float cy, float frameW, float frameH) {
        this.fx = fx;


        if (fy <= 1f || Math.abs(fx / fy - 1f) > 0.15f) {
            this.fy = fx;
        } else {
            this.fy = fy;
        }
        this.cx = cx;
        this.cy = cy;
        this.frameW = frameW;
        this.frameH = frameH;
        updateDrawMatrix();
        postInvalidateOnAnimation();
    }

    /** Calib RPY (deg) — same values / convention as model warp ({@link ModelCalibWarp}). */
    public void setCalibRpyDeg(float rollDeg, float pitchDeg, float yawDeg) {
        this.rollDeg = rollDeg;
        this.pitchDeg = pitchDeg;
        this.yawDeg = yawDeg;
        rebuildRt();
        postInvalidateOnAnimation();
    }


    public void setPreviewTransform(Matrix transform, int bufferW, int bufferH, int viewWidth, int viewHeight) {
        this.frameW = bufferW;
        this.frameH = bufferH;
        updateDrawMatrix();
        postInvalidateOnAnimation();
    }

    public void setCameraHeight(float meters) {
        this.cameraHeight = meters;
        postInvalidateOnAnimation();
    }


    public void setWaypointShift(float meters) {
        this.waypointShift = meters;
        postInvalidateOnAnimation();
    }

    public void setSteerRatio(float ratio) {
        this.steerRatio = ratio > 1f ? ratio : 15.7f;
        postInvalidateOnAnimation();
    }

    public void setLanes(LaneLines lanes) {
        this.lanes = lanes == null ? null : lanes.copy();
        postInvalidateOnAnimation();
    }


    public void setLaneKeep(boolean hasTarget, float targetX, float targetY,
                            float lookaheadM, float curvature, float steerRad, String status) {
        this.ppHasTarget = hasTarget;
        this.ppTargetX = targetX;
        this.ppTargetY = targetY;
        this.ppLookaheadM = lookaheadM;
        this.ppCurvature = curvature;
        this.ppSteerRad = steerRad;
        this.ppStatus = status == null ? "" : status;
        this.ppValid = true;
        postInvalidateOnAnimation();
    }


    public void setSteerCommand(int torqueCnm, boolean enabled) {
        this.torqueCnm = torqueCnm;
        this.steerEnabled = enabled;
        this.steerValid = true;
        postInvalidateOnAnimation();
    }

    public void setHcaStatus(String status) {
        this.hcaStatus = status == null ? "" : status;
        this.hcaValid = true;
        postInvalidateOnAnimation();
    }

    public void clearLaneKeep() {
        ppValid = false;
        steerValid = false;
        postInvalidateOnAnimation();
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        updateDrawMatrix();
    }

    private void updateDrawMatrix() {
        if (getWidth() <= 0 || getHeight() <= 0 || frameW <= 0 || frameH <= 0) {
            return;
        }
        float scale = Math.max(getWidth() / frameW, getHeight() / frameH);
        float dx = (getWidth() - frameW * scale) * 0.5f;
        float dy = (getHeight() - frameH * scale) * 0.5f;
        drawMatrix.reset();
        drawMatrix.setScale(scale, scale);
        drawMatrix.postTranslate(dx, dy);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        LaneLines ll = this.lanes;
        if (ll != null) {
            for (int i = 0; i < 2; i++) {
                drawPolylineXYZ(canvas, LaneLines.X_IDXS, ll.edgesY[i], ll.edgesZ[i],
                        edgePaint, 1.5f, false);
            }
            for (int i = 0; i < 4; i++) {
                if (ll.laneProbs[i] < MIN_LANE_PROB) {
                    continue;
                }
                lanePaint.setAlpha((int) (80 + 175 * Math.max(0f, Math.min(1f, ll.laneProbs[i]))));
                drawPolylineXYZ(canvas, LaneLines.X_IDXS, ll.lanesY[i], ll.lanesZ[i],
                        lanePaint, 1.5f, false);
            }
            if (ll.hasPlan) {
                drawPolylineXYZ(canvas, ll.planX, ll.planY, ll.planZ, pathPaint, 0.5f, true);
            }
        }

        if (ppValid) {
            drawPurePursuit(canvas);
            drawSteeringHud(canvas);
            drawPpStatus(canvas);
        }
        if (hcaValid && !hcaStatus.isEmpty()) {
            drawHcaStatus(canvas);
        }
    }

    private void drawPurePursuit(Canvas canvas) {
        final float raX = -waypointShift;
        final float raY = 0f;
        final float ld = Math.max(0.5f, ppLookaheadM);


        path.reset();
        boolean started = false;
        final int nCirc = 64;
        for (int i = 0; i <= nCirc; i++) {
            double th = 2.0 * Math.PI * i / nCirc;
            float x = raX + ld * (float) Math.cos(th);
            float y = raY + ld * (float) Math.sin(th);
            if (!projectEgo(x, y, 0.3f)) {
                started = false;
                continue;
            }
            if (!started) {
                path.moveTo(mapPt[0], mapPt[1]);
                started = true;
            } else {
                path.lineTo(mapPt[0], mapPt[1]);
            }
        }
        if (started) {
            canvas.drawPath(path, ppLdPaint);
        }


        float kappa = ppCurvature;
        float arcLen = Math.min(Math.max(ld * 1.5f, 15f), 50f);
        path.reset();
        started = false;
        final int nArc = 48;
        for (int i = 0; i < nArc; i++) {
            float s = arcLen * i / (nArc - 1);
            float ax;
            float ay;
            if (Math.abs(kappa) < 1e-6f) {
                ax = raX + s;
                ay = raY;
            } else {
                ax = raX + (float) (Math.sin(kappa * s) / kappa);
                ay = raY + (float) ((1.0 - Math.cos(kappa * s)) / kappa);
            }
            if (!projectEgo(ax, ay, 0.3f)) {
                started = false;
                continue;
            }
            if (!started) {
                path.moveTo(mapPt[0], mapPt[1]);
                started = true;
            } else {
                path.lineTo(mapPt[0], mapPt[1]);
            }
        }
        if (started) {
            canvas.drawPath(path, ppArcPaint);
        }


        if (projectEgo(raX, raY, -5f)) {
            float px = mapPt[0];
            float py = mapPt[1];
            float r = 10f;
            canvas.drawLine(px - r, py - r, px + r, py + r, ppLdPaint);
            canvas.drawLine(px - r, py + r, px + r, py - r, ppLdPaint);
        }


        if (ppHasTarget && projectEgo(ppTargetX, ppTargetY, 0.3f)) {
            float tx = mapPt[0];
            float ty = mapPt[1];
            if (projectEgo(raX, raY, -5f)) {
                canvas.drawLine(mapPt[0], mapPt[1], tx, ty, ppRayPaint);
            }
            canvas.drawCircle(tx, ty, 10f, ppTargetPaint);
            ppTargetPaint.setStyle(Paint.Style.STROKE);
            ppTargetPaint.setStrokeWidth(2f);
            ppTargetPaint.setColor(Color.WHITE);
            canvas.drawCircle(tx, ty, 12f, ppTargetPaint);
            ppTargetPaint.setStyle(Paint.Style.FILL);
            ppTargetPaint.setColor(Color.CYAN);
        }
    }

    private void drawSteeringHud(Canvas canvas) {
        float radius = Math.min(56f, getWidth() * 0.07f);
        float cxHud = getWidth() - radius - 24f;
        float cyHud = getHeight() - radius - 72f;

        canvas.drawCircle(cxHud, cyHud, radius + 8f, hudFillPaint);
        canvas.drawCircle(cxHud, cyHud, radius, hudPaint);
        canvas.drawCircle(cxHud, cyHud, radius * 0.35f, hudPaint);


        float wheelDeg = (float) Math.toDegrees(ppSteerRad) * steerRatio;
        wheelDeg = Math.max(-120f, Math.min(120f, wheelDeg));
        double ang = Math.toRadians(wheelDeg);

        for (float a0 : new float[]{90f, 210f, 330f}) {
            double a = Math.toRadians(a0);
            float px = radius * 0.92f * (float) Math.cos(a);
            float py = -radius * 0.92f * (float) Math.sin(a);
            float[] p1 = rotateHud(px, py, ang);
            canvas.drawLine(cxHud, cyHud, cxHud + p1[0], cyHud + p1[1], hudPaint);
        }

        float[] top = rotateHud(0f, -radius * 0.85f, ang);
        Paint hub = new Paint(Paint.ANTI_ALIAS_FLAG);
        hub.setColor(Color.rgb(255, 200, 0));
        hub.setStyle(Paint.Style.FILL);
        canvas.drawCircle(cxHud, cyHud, 6f, hub);
        hub.setColor(Color.RED);
        canvas.drawCircle(cxHud + top[0], cyHud + top[1], 6f, hub);

        textPaint.setTextSize(26f);
        textPaint.setColor(Color.rgb(255, 200, 0));
        String road = String.format("%+.1f° road", Math.toDegrees(ppSteerRad));
        String sw = String.format("SW %+.0f°", wheelDeg);
        canvas.drawText(road, cxHud - radius, cyHud + radius + 28f, textPaint);
        textPaint.setColor(Color.LTGRAY);
        canvas.drawText(sw, cxHud - radius, cyHud + radius + 54f, textPaint);


        if (steerValid) {
            float barW = 14f;
            float barH = radius * 2f;
            float barX = cxHud - radius - 28f;
            float barY = cyHud - radius;
            Paint barBg = new Paint(Paint.ANTI_ALIAS_FLAG);
            barBg.setColor(Color.argb(160, 40, 40, 40));
            canvas.drawRect(barX, barY, barX + barW, barY + barH, barBg);
            float mid = barY + barH * 0.5f;
            float frac = Math.max(-1f, Math.min(1f, torqueCnm / MAX_TORQUE_CNM));
            Paint barFg = new Paint(Paint.ANTI_ALIAS_FLAG);
            barFg.setColor(steerEnabled ? Color.rgb(0, 220, 120) : Color.rgb(180, 180, 80));
            if (frac >= 0) {
                canvas.drawRect(barX, mid - frac * barH * 0.5f, barX + barW, mid, barFg);
            } else {
                canvas.drawRect(barX, mid, barX + barW, mid - frac * barH * 0.5f, barFg);
            }
            Paint midLine = new Paint(Paint.ANTI_ALIAS_FLAG);
            midLine.setColor(Color.WHITE);
            midLine.setStrokeWidth(2f);
            canvas.drawLine(barX - 2f, mid, barX + barW + 2f, mid, midLine);

            textPaint.setTextSize(22f);
            textPaint.setColor(steerEnabled ? Color.rgb(0, 220, 120) : Color.LTGRAY);
            canvas.drawText(
                    String.format("%s %d cNm", steerEnabled ? "TQ" : "off", torqueCnm),
                    barX - 8f,
                    barY - 8f,
                    textPaint);
        }
    }

    private static float[] rotateHud(float px, float py, double ang) {
        double c = Math.cos(ang);
        double s = Math.sin(ang);

        return new float[]{(float) (c * px + s * py), (float) (-s * px + c * py)};
    }

    private void drawPpStatus(Canvas canvas) {
        float kappa = ppCurvature;
        String curv;
        if (Math.abs(kappa) < 1e-6f) {
            curv = "κ=0  R=∞";
        } else {
            curv = String.format("κ=%.4f/m  R=%.1fm", kappa, 1.0 / Math.abs(kappa));
        }
        String line1 = String.format(
                "PP Ld=%.1fm  δ=%+.1f°  %s  %s",
                ppLookaheadM,
                Math.toDegrees(ppSteerRad),
                curv,
                ppHasTarget ? "" : "(no target)");
        String line2 = ppStatus.isEmpty() ? "magenta=arc  orange=Ld  cyan=target" : ("status=" + ppStatus);

        textPaint.setTextSize(26f);
        textPaint.setColor(Color.rgb(255, 165, 0));
        float pad = 8f;
        float x = 12f;
        float y = 40f;
        float w = Math.max(textPaint.measureText(line1), textPaint.measureText(line2)) + pad * 2;
        canvas.drawRect(x - pad, y - 28f, x + w, y + 36f, textBgPaint);
        canvas.drawText(line1, x, y, textPaint);
        textPaint.setTextSize(22f);
        textPaint.setColor(Color.rgb(200, 150, 200));
        canvas.drawText(line2, x, y + 28f, textPaint);
    }

    private void drawHcaStatus(Canvas canvas) {
        textPaint.setTextSize(24f);
        boolean ok = hcaStatus.startsWith("HCA ok");
        textPaint.setColor(ok ? Color.rgb(0, 220, 120) : Color.rgb(255, 120, 80));
        float pad = 8f;
        float x = 12f;
        float y = ppValid ? 108f : 40f;
        float w = textPaint.measureText(hcaStatus) + pad * 2;
        canvas.drawRect(x - pad, y - 26f, x + w, y + 10f, textBgPaint);
        canvas.drawText(hcaStatus, x, y, textPaint);
    }


    private void drawPolylineXYZ(Canvas canvas, float[] xs, float[] ys, float[] zs,
                                 Paint paint, float xMin, boolean pathLift) {
        path.reset();
        boolean started = false;
        int n = Math.min(xs.length, ys.length);
        if (zs != null) {
            n = Math.min(n, zs.length);
        }
        for (int i = 0; i < n; i++) {
            float X = xs[i];
            float Z = zs != null ? zs[i] : 0f;
            if (X < xMin || !Float.isFinite(X) || !Float.isFinite(ys[i]) || !Float.isFinite(Z)) {
                started = false;
                continue;
            }
            if (!projectDevice(X, ys[i], Z, xMin, pathLift)) {
                started = false;
                continue;
            }
            float px = mapPt[0];
            float py = mapPt[1];
            if (!started) {
                path.moveTo(px, py);
                started = true;
            } else {
                path.lineTo(px, py);
            }
        }
        if (started) {
            canvas.drawPath(path, paint);
        }
    }

    /**
     * Remap device (X,Y,Z)→(Y,Z,X) then apply Rt. Device: X fwd, Y right+, Z up.
     * Rt matches warp: V·R(rpy)·V⁻¹ with R from {@link ModelCalibWarp#rotFromEuler}.
     */
    private boolean projectDevice(float X, float Y, float Z, float xMin, boolean pathLift) {
        if (X < xMin || !Float.isFinite(X) || !Float.isFinite(Y) || !Float.isFinite(Z)) {
            return false;
        }
        float camX = Y;
        float camY = Z + (pathLift ? PATH_LIFT_M : 0f);
        float camZ = X;
        float x = r00 * camX + r01 * camY + r02 * camZ;
        float y = r10 * camX + r11 * camY + r12 * camZ;
        float z = r20 * camX + r21 * camY + r22 * camZ;
        if (z < 0.15f) {
            return false;
        }
        float u = fx * (x / z) + cx;
        float v = fy * (y / z) + cy;
        mapPt[0] = u;
        mapPt[1] = v;
        drawMatrix.mapPoints(mapPt);
        float px = mapPt[0];
        float py = mapPt[1];
        return !(px < -80 || px > getWidth() + 80 || py < -80 || py > getHeight() + 80);
    }

    /** PP / HUD points at ~camera height in device-Z (like Draw lead at 1.32). */
    private boolean projectEgo(float X, float Y, float xMin) {
        return projectDevice(X, Y, cameraHeight, xMin, false);
    }

    /**
     * Keep overlay RPY axes identical to {@link ModelCalibWarp} / Preprocess.
     * LibGDX {@code setFromEulerAnglesRad(-pitch,-yaw,-roll)} mixes pitch↔yaw
     * relative to that convention (slider Pitch looked like Yaw on the HUD).
     */
    private void rebuildRt() {
        float[] R = ModelCalibWarp.rotFromEuler(
                Math.toRadians(rollDeg),
                Math.toRadians(pitchDeg),
                Math.toRadians(yawDeg));
        // view_from_device
        float[] V = {
                0, 1, 0,
                0, 0, 1,
                1, 0, 0
        };
        // V⁻¹ = Vᵀ for this permutation matrix
        float[] Vi = {
                0, 0, 1,
                1, 0, 0,
                0, 1, 0
        };
        float[] Rt = ModelCalibWarp.mul3(ModelCalibWarp.mul3(V, R), Vi);
        r00 = Rt[0];
        r01 = Rt[1];
        r02 = Rt[2];
        r10 = Rt[3];
        r11 = Rt[4];
        r12 = Rt[5];
        r20 = Rt[6];
        r21 = Rt[7];
        r22 = Rt[8];
    }
}
