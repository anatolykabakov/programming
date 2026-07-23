package ai.flow.adas.vision;

import android.content.Context;
import android.graphics.Bitmap;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;

import ai.flow.adas.Logger;
import ai.flow.adas.Messages;
import ai.flow.adas.ProtoUtils;
import ai.flow.adas.ZMQBridgeService;

public class VisionPipeline {
    private static final String TAG = "VisionPipeline";

    private final HandlerThread thread;
    private final Handler handler;
    private final SupercomboOnnxRunner runner;
    private final LaneOverlayView overlay;
    private final boolean publishPose;

    private volatile boolean busy;
    private int frameId;

    public VisionPipeline(Context context, LaneOverlayView overlay) throws Exception {
        this(context, overlay, true);
    }

    public VisionPipeline(Context context, LaneOverlayView overlay, boolean cameraCalib)
            throws Exception {
        this.overlay = overlay;
        this.publishPose = cameraCalib;
        this.runner = new SupercomboOnnxRunner(context.getApplicationContext());
        thread = new HandlerThread("SupercomboInfer");
        thread.start();
        handler = new Handler(thread.getLooper());
    }

    public void submitBitmap(Bitmap bitmap) {
        submitBitmap(bitmap, ai.flow.adas.TimeUtil.nowMs());
    }

    /** @param captureTsMs BOOTTIME ms when the camera frame arrived / was submitted */
    public void submitBitmap(Bitmap bitmap, long captureTsMs) {
        if (busy || bitmap == null) {
            return;
        }
        busy = true;
        final Bitmap copy = bitmap.copy(Bitmap.Config.ARGB_8888, false);
        final int id = frameId++;
        final long captureTs = captureTsMs > 0 ? captureTsMs : ai.flow.adas.TimeUtil.nowMs();
        handler.post(() -> {
            try {
                SupercomboOnnxRunner.Result res = runner.run(copy, id, captureTs);
                if (res == null || res.lanes == null) {
                    return;
                }
                LaneLines lanes = res.lanes;
                overlay.setLanes(lanes);
                Messages.ZMQMessage bagMsg = ProtoUtils.createLaneLinesMessage(lanes, true);
                if (bagMsg != null) {
                    Logger.getInstance().logZMQMessage(bagMsg);
                }
                Messages.ZMQMessage ctrlMsg = ProtoUtils.createLaneLinesMessage(lanes, false);
                if (ctrlMsg != null) {
                    ZMQBridgeService.publishToNative(ctrlMsg);
                }
                if (publishPose && res.pose != null && res.pose.valid) {
                    Messages.ZMQMessage poseMsg =
                            ProtoUtils.createCameraOdometryMessage(lanes.timestampMs, id, res.pose);
                    if (poseMsg != null) {
                        ZMQBridgeService.publishToNative(poseMsg);
                        Logger.getInstance().logZMQMessage(poseMsg);
                    }
                }
            } catch (Exception e) {
                Log.e(TAG, "infer failed", e);
            } finally {
                copy.recycle();
                busy = false;
            }
        });
    }

    public void setCalib(float rollDeg, float pitchDeg, float yawDeg,
                         float fx, float fy, float cx, float cy,
                         int width, int height) {
        runner.setCalib(rollDeg, pitchDeg, yawDeg, fx, fy, cx, cy, width, height);
    }

    public void close() {
        handler.post(runner::close);
        thread.quitSafely();
    }
}
