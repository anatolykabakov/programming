package ai.flow.android;

import static android.hardware.camera2.CameraMetadata.CONTROL_AF_MODE_AUTO;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.TotalCaptureResult;
import android.hardware.camera2.params.OutputConfiguration;
import android.hardware.camera2.params.SessionConfiguration;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.util.Range;
import android.view.Surface;
import android.graphics.SurfaceTexture;
import android.view.TextureView;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.nio.ByteBuffer;

import ai.flow.android.Messages.ZMQMessage;
import messaging.ZMQPubHandler;

public class CameraHandler {

    private final String TAG = "CameraHandler";

    private final Context context;
    private HandlerThread backgroundThread;
    private Handler backgroundHandler;
    private ImageReader reader;
    private CaptureRequest.Builder captureRequest;
    private CameraCaptureSession captureSession;
    private CameraDevice cameraDevice;
    private CameraCharacteristics cameraCharacteristics;

    private ZMQPubHandler pubHandler;

    TextureView preview;

    KITTIDataset dataset;
    
    // ZMQ variables
    public int W = 1280;
    public int H = 720;
    public int frameID = 0;

    public CameraHandler(Context context, TextureView preview) {
        this.context = context;
        this.preview = preview;
        this.dataset = KITTIDataset.getInstance();
        this.pubHandler = new ZMQPubHandler(context);
        backgroundThread = new HandlerThread("CameraBackground");
        backgroundThread.start();
        backgroundHandler = new Handler(backgroundThread.getLooper());

        // Initialize ZMQ publishers
        boolean zmqInit = pubHandler.createPublishers(Arrays.asList("wideRoadCameraBuffer", "wideRoadCameraState"));
        if (!zmqInit) {
            Log.e(TAG, "Failed to initialize ZMQ publishers");
        }
        
        // Initialize ZMQ message objects
    }

    public void stop(){
        if (cameraDevice != null) {
            cameraDevice.close();
            cameraDevice = null;
        }
        if (pubHandler != null) {
            pubHandler.releaseAll();
        }
    }

    public void start() {
        android.hardware.camera2.CameraManager manager = (android.hardware.camera2.CameraManager) context.getSystemService(Context.CAMERA_SERVICE);

        if (manager == null) {
            throw new RuntimeException("Unable to get camera manager.");
        }

        String cameraId = "0";

        try {
            cameraCharacteristics = manager.getCameraCharacteristics(cameraId);
            StreamConfigurationMap map = cameraCharacteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);

            if (ActivityCompat.checkSelfPermission(context, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                return;
            }

            manager.openCamera(cameraId, new CameraDevice.StateCallback() {
                @Override
                public void onOpened(@NonNull CameraDevice device) {
                    cameraDevice = device;
                    startCamera();
                }

                @Override
                public void onDisconnected(@NonNull CameraDevice device) {}

                @Override
                public void onError(@NonNull CameraDevice device, int error) {
                    Log.w(TAG, "Error opening camera: " + error);
                }
            }, backgroundHandler);
        } catch (CameraAccessException e) {
            Log.w(TAG, "Error getting camera configuration.", e);
        }
    }

    private void startCamera() {
        List<Surface> list = new ArrayList<>();

        final int width = 1280, height = 720;
        reader = ImageReader.newInstance(width, height, ImageFormat.YUV_420_888, 5);

        SurfaceTexture texture = preview.getSurfaceTexture();
        texture.setDefaultBufferSize(width, height);
        Surface previewSurface = new Surface(texture);

        list.add(reader.getSurface());
        list.add(previewSurface);

        ImageReader.OnImageAvailableListener imageAvailableListener = new ImageReader.OnImageAvailableListener() {
            @Override
            public void onImageAvailable(ImageReader reader) {
                try {
                    Image image = reader.acquireLatestImage();
                    if (image == null) return;
                    
                    // Send via ZMQ
                    sendImageViaZMQ(image);
                    
                    image.close();
                } catch (Throwable t) {
                    t.printStackTrace();
                }
            }
        };

        reader.setOnImageAvailableListener(imageAvailableListener, backgroundHandler);

        try {
            captureRequest = cameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_RECORD);
            captureRequest.addTarget(list.get(0));
            captureRequest.addTarget(previewSurface);
            Integer afMode = CONTROL_AF_MODE_AUTO;//afMode(cameraCharacteristics);
            captureRequest.set(CaptureRequest.CONTROL_AF_TRIGGER, CaptureRequest.CONTROL_AF_TRIGGER_CANCEL);
            captureRequest.set(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE, new Range<>(20, 20));

            if (afMode != null) {
                captureRequest.set(CaptureRequest.CONTROL_AF_MODE, afMode);
                Log.i(TAG, "Setting af mode to: " + afMode);
                if (afMode == CONTROL_AF_MODE_AUTO) {
                    captureRequest.set(CaptureRequest.CONTROL_AF_TRIGGER, CaptureRequest.CONTROL_AF_TRIGGER_START);
                } else {
                    captureRequest.set(CaptureRequest.CONTROL_AF_TRIGGER, CaptureRequest.CONTROL_AF_TRIGGER_CANCEL);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
            return;
        }

        try {
            List<OutputConfiguration> confs = new ArrayList<>();
            for (Surface surface : list) {
                confs.add(new OutputConfiguration(surface));
            }

            cameraDevice.createCaptureSession(
                    new SessionConfiguration(
                            SessionConfiguration.SESSION_REGULAR,
                            confs,
                            context.getMainExecutor(),
                            new CameraCaptureSession.StateCallback() {
                                @Override
                                public void onConfigured(CameraCaptureSession session) {
                                    captureSession = session;
                                    startSession();
                                }

                                @Override
                                public void onConfigureFailed(CameraCaptureSession session) {
                                    System.out.println("### Configuration Fail ###");
                                }
                            }
                    )
            );
        } catch (Throwable t) {
            t.printStackTrace();
        }
    }

    private void startSession() {
        CameraCaptureSession.CaptureCallback listener = new CameraCaptureSession.CaptureCallback() {
            public void onCaptureCompleted(CameraCaptureSession session, CaptureRequest request, TotalCaptureResult result) {
                super.onCaptureCompleted(session, request, result);
            }
        };

        if (cameraDevice == null) return;

        try {
            captureSession.setRepeatingRequest(captureRequest.build(), listener, backgroundHandler);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    private void sendImageViaZMQ(Image image) {
        try {
            // Convert image to YUV format
            ByteBuffer yuvBuffer = ByteBuffer.allocateDirect(W * H * 3 / 2);
            Utils.fillYUVBuffer(image, yuvBuffer);
            
            // Convert ByteBuffer to byte array
            byte[] yuvData = new byte[yuvBuffer.position()];
            yuvBuffer.rewind();
            yuvBuffer.get(yuvData);
            
            long timestamp = System.currentTimeMillis();
            
            // Create protobuf camera image message
            ZMQMessage imageMessage = ProtobufUtils.createCameraImageMessage(
                yuvData, W, H, "YUV420", frameID, timestamp);
            
            // Create protobuf camera state message
            ZMQMessage stateMessage = ProtobufUtils.createCameraStateMessage(
                frameID, true, true, timestamp);
            
            // Serialize and publish via ZMQ
            byte[] imageData = ProtobufUtils.serializeMessage(imageMessage);
            byte[] stateData = ProtobufUtils.serializeMessage(stateMessage);
            
            // Publish image data via ZMQ
            if (pubHandler != null) {
                pubHandler.publish("wideRoadCameraBuffer", imageData);
                pubHandler.publish("wideRoadCameraState", stateData);
            } else {
                Log.w(TAG, "Publisher not ready, skipping ZMQ publish");
            }
            
            // Log message info
            ProtobufUtils.logMessageInfo(imageMessage);
            
            frameID++;
            
        } catch (Exception e) {
            Log.e(TAG, "Error sending image via ZMQ", e);
        }
    }
    
    public void cleanup() {
        if (pubHandler != null) {
            pubHandler.releaseAll();
        }
    }
}
