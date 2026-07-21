package ai.flow.adas.vision;

import android.content.Context;
import android.graphics.Bitmap;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;

import ai.flow.adas.Logger;
import ai.flow.adas.Messages;
import ai.flow.adas.ProtoUtils;

/**
 * ONNX runner thread: Bitmap → LaneLines + CameraOdometry → overlay / ZMQ.
 * Live calib algorithm lives in C++ CameraCalibService (flowpilot pose path).
 */
public class VisionPipeline {
    private static final String TAG = "VisionPipeline";

    private final HandlerThread thread;
    private final Handler handler;
    private final SupercomboOnnxRunner runner;
    private final LanePublisher publisher;
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
        this.publisher = new LanePublisher();
        thread = new HandlerThread("SupercomboInfer");
        thread.start();
        handler = new Handler(thread.getLooper());
    }

    public void submitBitmap(Bitmap bitmap) {
        if (busy || bitmap == null) {
            return;
        }
        busy = true;
        final Bitmap copy = bitmap.copy(Bitmap.Config.ARGB_8888, false);
        final int id = frameId++;
        handler.post(() -> {
            try {
                SupercomboOnnxRunner.Result res = runner.run(copy, id);
                if (res == null || res.lanes == null) {
                    return;
                }
                LaneLines lanes = res.lanes;
                overlay.setLanes(lanes);
                publisher.publish(lanes);
                Messages.ZMQMessage bagMsg = ProtoUtils.createLaneLinesMessage(lanes);
                if (bagMsg != null) {
                    Logger.getInstance().logZMQMessage(bagMsg);
                }
                if (publishPose && res.pose != null && res.pose.valid) {
                    Messages.ZMQMessage poseMsg =
                            ProtoUtils.createCameraOdometryMessage(lanes.timestampMs, id, res.pose);
                    if (poseMsg != null) {
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

    public void close() {
        handler.post(() -> {
            runner.close();
            publisher.close();
        });
        thread.quitSafely();
    }
}
