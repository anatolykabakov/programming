package ai.flow.adas.vision;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.util.AttributeSet;
import android.view.View;

/**
 * Projects ego-frame supercombo outputs onto the camera view.
 * Yellow = lane lines, red = road edges, green = PLAN (best MHP path).
 *
 * Uses center-crop of the capture buffer (W×H) into the view — same aspect as the
 * corrected TextureView preview. Do NOT reuse TextureView.setTransform(): that matrix
 * is in TextureView's internal space and rotates overlay points incorrectly.
 */
public class LaneOverlayView extends View {
    private static final float MIN_LANE_PROB = 0.3f;

    private final Paint lanePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint edgePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint pathPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
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
        setWillNotDraw(false);
    }

    public void setIntrinsics(float fx, float fy, float cx, float cy, float frameW, float frameH) {
        this.fx = fx;
        // Square pixels: anamorphic fy from full-sensor mapping lifts lines off the road
        // by roughly camera-height in the image.
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

    /**
     * Kept for CameraHandler API compatibility; TextureView matrix is intentionally ignored.
     * Overlay always center-crops buffer W×H into the view.
     */
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

    public void setLanes(LaneLines lanes) {
        this.lanes = lanes == null ? null : lanes.copy();
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
        if (ll == null) {
            return;
        }

        for (int i = 0; i < 2; i++) {
            drawPolylineXY(canvas, LaneLines.X_IDXS, ll.edgesY[i], edgePaint, 1.5f);
        }
        for (int i = 0; i < 4; i++) {
            if (ll.laneProbs[i] < MIN_LANE_PROB) {
                continue;
            }
            lanePaint.setAlpha((int) (80 + 175 * Math.max(0f, Math.min(1f, ll.laneProbs[i]))));
            drawPolylineXY(canvas, LaneLines.X_IDXS, ll.lanesY[i], lanePaint, 1.5f);
        }
        if (ll.hasPlan) {
            drawPolylineXY(canvas, ll.planX, ll.planY, pathPaint, 0.5f);
        }
    }

    /**
     * Project ego (X forward, Y left — openpilot) with pinhole:
     *   u = cx - fx * Y / X   (+Y left → smaller u)
     *   v = cy + fy * h / X
     *
     * Lateral Y is already converted to Y-left in {@link SupercomboOnnxRunner}
     * (this ONNX raw output is Y-right).
     */
    private void drawPolylineXY(Canvas canvas, float[] xs, float[] ys, Paint paint, float xMin) {
        path.reset();
        boolean started = false;
        int n = Math.min(xs.length, ys.length);
        for (int i = 0; i < n; i++) {
            float X = xs[i];
            if (X < xMin || !Float.isFinite(X) || !Float.isFinite(ys[i])) {
                started = false;
                continue;
            }
            float Y = ys[i];
            float u = cx - fx * (Y / X);
            float v = cy + fy * (cameraHeight / X);

            mapPt[0] = u;
            mapPt[1] = v;
            drawMatrix.mapPoints(mapPt);
            float px = mapPt[0];
            float py = mapPt[1];

            if (px < -80 || px > getWidth() + 80 || py < -80 || py > getHeight() + 80) {
                started = false;
                continue;
            }
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
}
