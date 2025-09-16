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
import android.graphics.Matrix;
import android.util.Size;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;

import android.graphics.Bitmap;
import ai.flow.android.Messages.ZMQMessage;
import ai.flow.android.ProtoUtils;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;


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


    TextureView preview;

    // ZMQ variables
    public int W = 1280;
    public int H = 720;
    public int frameID = 0;
    
    // Image compression settings
    private static final int JPEG_QUALITY = 70; // 0-100, lower = smaller file
    private static final int SCALE_FACTOR = 2; // Scale down by this factor (2 = half size)

    public CameraHandler(Context context, TextureView preview) {
        this.context = context;
        this.preview = preview;
        backgroundThread = new HandlerThread("CameraBackground");
        backgroundThread.start();
        backgroundHandler = new Handler(backgroundThread.getLooper());
    }

    public void stop(){
        if (cameraDevice != null) {
            cameraDevice.close();
            cameraDevice = null;
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

        reader = ImageReader.newInstance(W, H, ImageFormat.YUV_420_888, 5);

        SurfaceTexture texture = preview.getSurfaceTexture();
        texture.setDefaultBufferSize(W, H);
        Surface previewSurface = new Surface(texture);

        list.add(reader.getSurface());
        list.add(previewSurface);

        ImageReader.OnImageAvailableListener imageAvailableListener = new ImageReader.OnImageAvailableListener() {
            @Override
            public void onImageAvailable(ImageReader reader) {
                try {
                    Image image = reader.acquireLatestImage();
                    if (image == null) return;

                    // Convert Image to grayscale Bitmap (most efficient for ADAS/logging)
                    Bitmap bitmap = convertImageToGrayscaleBitmap(image);
                    if (bitmap != null) {
                        // Scale down image to reduce storage (4x smaller file size)
                        Bitmap scaledBitmap = Bitmap.createScaledBitmap(bitmap, W/SCALE_FACTOR, H/SCALE_FACTOR, true);
                        bitmap.recycle(); // Free original bitmap memory
                        bitmap = scaledBitmap;
                        
                        long currentTime = System.currentTimeMillis();
                        
                        // Логируем только если Logger запущен
                        if (Logger.getInstance().isRunning()) {
                            // Логируем изображение в файл (как раньше)
                            // Logger.getInstance().logCameraImage(currentTime, bitmap);
                            
                            // Создаем protobuf сообщение для bag логирования
                            try {
                            // Конвертируем Bitmap в byte array с уменьшенным качеством для экономии места
                                java.io.ByteArrayOutputStream stream = new java.io.ByteArrayOutputStream();
                                bitmap.compress(Bitmap.CompressFormat.JPEG, JPEG_QUALITY, stream);
                                byte[] imageData = stream.toByteArray();
                                
                                // Конвертируем byte[] в List<Byte>
                                java.util.List<Byte> imageDataList = new java.util.ArrayList<>();
                                for (byte b : imageData) {
                                    imageDataList.add(b);
                                }
                                
                                // Создаем ZMQMessage с Camera данными
                                ZMQMessage cameraMessage = ProtoUtils.createCameraImageMessage(
                                    imageDataList, 
                                    bitmap.getWidth(), 
                                    bitmap.getHeight(), 
                                    "JPEG", 
                                    frameID++, 
                                    currentTime);

                                // Логируем в bag файл
                                Logger.getInstance().logZMQMessage(cameraMessage);
                            
                                Log.d(TAG, String.format("Camera image logged: %dx%d, %d bytes (compressed %dx)", 
                                    bitmap.getWidth(), bitmap.getHeight(), imageData.length, SCALE_FACTOR));
                            } catch (Exception e) {
                                Log.e(TAG, "Error creating camera protobuf message for bag logging", e);
                            }
                        } else {
                            Log.d(TAG, "Logger not running, skipping camera image logging");
                        }
                    }

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

            // ========== CRITICAL SETTINGS FOR CAMERA CALIBRATION ==========

            // 1. FIXED FOCUS - Most important for calibration
            // Disable auto focus and set focus to infinity (0.0f = infinity, for ADAS)
            captureRequest.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_OFF);

            // Check if manual focus distance is supported
            Float minFocusDistance = cameraCharacteristics.get(CameraCharacteristics.LENS_INFO_MINIMUM_FOCUS_DISTANCE);
            if (minFocusDistance != null && minFocusDistance > 0) {
                // 0.0f = infinity (best for ADAS - road/far objects)
                captureRequest.set(CaptureRequest.LENS_FOCUS_DISTANCE, 0.0f);
                Log.i(TAG, "Set focus to infinity (0.0f) for calibration. Min focus distance: " + minFocusDistance);
            } else {
                Log.w(TAG, "Manual focus distance not supported on this device");
            }

            // 2. DISABLE OPTICAL IMAGE STABILIZATION (OIS)
            // OIS physically moves lens elements, changing optical center
            int[] availableOIS = cameraCharacteristics.get(CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION);
            if (availableOIS != null && availableOIS.length > 0) {
                captureRequest.set(CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE,
                                 CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE_OFF);
                Log.i(TAG, "Disabled optical image stabilization (OIS) for calibration");
            } else {
                Log.i(TAG, "Optical image stabilization (OIS) not available on this device");
            }

            // 3. DISABLE VIDEO STABILIZATION (digital stabilization)
            // Digital stabilization crops and scales image, changing intrinsic parameters
            int[] availableVideoStab = cameraCharacteristics.get(CameraCharacteristics.CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES);
            if (availableVideoStab != null && availableVideoStab.length > 0) {
                captureRequest.set(CaptureRequest.CONTROL_VIDEO_STABILIZATION_MODE,
                                 CaptureRequest.CONTROL_VIDEO_STABILIZATION_MODE_OFF);
                Log.i(TAG, "Disabled video stabilization for calibration");
            } else {
                Log.i(TAG, "Video stabilization not available on this device");
            }

            // 4. DISABLE LENS DISTORTION CORRECTION (Android P+)
            // We want raw distortion for calibration, not corrected images
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.P) {
                int[] availableDistortionModes = cameraCharacteristics.get(CameraCharacteristics.DISTORTION_CORRECTION_AVAILABLE_MODES);
                if (availableDistortionModes != null && availableDistortionModes.length > 0) {
                    captureRequest.set(CaptureRequest.DISTORTION_CORRECTION_MODE,
                                     CaptureRequest.DISTORTION_CORRECTION_MODE_OFF);
                    Log.i(TAG, "Disabled lens distortion correction for calibration (raw distortion preserved)");
                } else {
                    Log.i(TAG, "Distortion correction modes not available on this device");
                }
            }

            // ========== OPTIONAL SETTINGS ==========

            // Frame rate
            captureRequest.set(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE, new Range<>(10, 10));
            Log.i(TAG, "Set target FPS to 10");

            // Log camera intrinsic parameters for calibration reference
            logCameraIntrinsics();

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

    /**
     * Log camera intrinsic parameters that are needed for calibration.
     * These values should be saved with calibration images for reference.
     */
    private void logCameraIntrinsics() {
        try {
            StringBuilder intrinsicsData = new StringBuilder();
            intrinsicsData.append("=== CAMERA INTRINSIC PARAMETERS ===\n");

            // Focal length in mm (physical)
            float[] focalLengths = cameraCharacteristics.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS);
            if (focalLengths != null && focalLengths.length > 0) {
                intrinsicsData.append("Physical focal length: ").append(focalLengths[0]).append(" mm\n");
            }

            // Sensor physical size in mm
            android.util.SizeF sensorSize = cameraCharacteristics.get(CameraCharacteristics.SENSOR_INFO_PHYSICAL_SIZE);
            if (sensorSize != null) {
                intrinsicsData.append("Sensor physical size: ").append(sensorSize.getWidth())
                              .append(" x ").append(sensorSize.getHeight()).append(" mm\n");
            }

            // Active pixel array size (actual image resolution from sensor)
            android.graphics.Rect activeArray = cameraCharacteristics.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE);
            if (activeArray != null) {
                intrinsicsData.append("Active pixel array: ").append(activeArray.width())
                              .append(" x ").append(activeArray.height()).append(" pixels\n");
            }

            // Lens distortion coefficients (if available on Android P+)
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.P) {
                float[] distortion = cameraCharacteristics.get(CameraCharacteristics.LENS_DISTORTION);
                if (distortion != null && distortion.length >= 5) {
                    intrinsicsData.append("Lens distortion coefficients: [")
                                  .append(distortion[0]).append(", ")
                                  .append(distortion[1]).append(", ")
                                  .append(distortion[2]).append(", ")
                                  .append(distortion[3]).append(", ")
                                  .append(distortion[4]).append("]\n");
                    intrinsicsData.append("Distortion model: k1, k2, k3, k4, k5 (radial + tangential)\n");
                }
            }

            // Lens intrinsic calibration (if available on Android P+)
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.P) {
                float[] intrinsicCalibration = cameraCharacteristics.get(CameraCharacteristics.LENS_INTRINSIC_CALIBRATION);
                if (intrinsicCalibration != null && intrinsicCalibration.length >= 5) {
                    intrinsicsData.append("Lens intrinsic calibration: [")
                                  .append(intrinsicCalibration[0]).append(", ")
                                  .append(intrinsicCalibration[1]).append(", ")
                                  .append(intrinsicCalibration[2]).append(", ")
                                  .append(intrinsicCalibration[3]).append(", ")
                                  .append(intrinsicCalibration[4]).append("]\n");
                    intrinsicsData.append("Format: [fx, fy, cx, cy, s] where s=skew\n");
                }
            }

            // Calculate approximate focal length in pixels
            if (focalLengths != null && focalLengths.length > 0 &&
                sensorSize != null && activeArray != null) {
                float focalLengthMm = focalLengths[0];
                float sensorWidthMm = sensorSize.getWidth();
                int imageWidthPx = activeArray.width();

                float focalLengthPx = (focalLengthMm / sensorWidthMm) * imageWidthPx;
                intrinsicsData.append("Calculated focal length in pixels (fx): ").append(focalLengthPx).append(" px\n");
                intrinsicsData.append("Note: This is approximate. Use calibration for accurate values.\n");
            }

            // Image resolution we're actually capturing
            intrinsicsData.append("Capture resolution: ").append(W).append(" x ").append(H).append(" pixels\n");
            intrinsicsData.append("=== End of camera intrinsics ===");

            // Logger.getInstance().logCameraI ntrinsics(intrinsicsData.toString());

            // Создаем ZMQ сообщение для bag логирования
            if (Logger.getInstance().isRunning()) {
                try {
                    long currentTime = System.currentTimeMillis();
                    
                    // Извлекаем данные для protobuf
                    float physicalFocalLengthMm = 0.0f;
                    float sensorWidthMm = 0.0f, sensorHeightMm = 0.0f;
                    int activeArrayWidth = 0, activeArrayHeight = 0;
                    float[] distortionCoefficients = null;
                    float[] intrinsicCalibration = null;
                    float focalLengthPx = 0.0f;
                    String distortionModel = "radial_tangential";
                    
                    // Focal length
                    if (focalLengths != null && focalLengths.length > 0) {
                        physicalFocalLengthMm = focalLengths[0];
                    }
                    
                    // Sensor size
                    if (sensorSize != null) {
                        sensorWidthMm = sensorSize.getWidth();
                        sensorHeightMm = sensorSize.getHeight();
                    }
                    
                    // Active array
                    if (activeArray != null) {
                        activeArrayWidth = activeArray.width();
                        activeArrayHeight = activeArray.height();
                    }
                    
                    // Distortion coefficients
                    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.P) {
                        distortionCoefficients = cameraCharacteristics.get(CameraCharacteristics.LENS_DISTORTION);
                        intrinsicCalibration = cameraCharacteristics.get(CameraCharacteristics.LENS_INTRINSIC_CALIBRATION);
                    }
                    
                    // Calculate focal length in pixels
                    if (focalLengths != null && focalLengths.length > 0 &&
                        sensorSize != null && activeArray != null) {
                        float focalLengthMm = focalLengths[0];
                        int imageWidthPx = activeArray.width();
                        focalLengthPx = (focalLengthMm / sensorWidthMm) * imageWidthPx;
                    }
                    
                    // Создаем ZMQ сообщение
                    Messages.ZMQMessage intrinsicsMessage = ProtoUtils.createCameraIntrinsicsMessage(
                        physicalFocalLengthMm,
                        sensorWidthMm, sensorHeightMm,
                        activeArrayWidth, activeArrayHeight,
                        distortionCoefficients,
                        intrinsicCalibration,
                        focalLengthPx,
                        W, H, // capture resolution
                        "camera_0", // camera ID
                        distortionModel,
                        currentTime);
                    
                    // Логируем в bag файл
                    Logger.getInstance().logZMQMessage(intrinsicsMessage);
                    
                    Log.d(TAG, "Camera intrinsics logged to both text file and bag file");
                } catch (Exception e) {
                    Log.e(TAG, "Error creating camera intrinsics protobuf message for bag logging", e);
                }
            }

        } catch (Exception e) {
            String errorMsg = "Error logging camera intrinsics: " + e.getMessage();
            Log.e(TAG, errorMsg, e);
        }
    }

    /**
     * Convert Android Image (YUV_420_888) to grayscale Bitmap (most efficient)
     */
    private Bitmap convertImageToGrayscaleBitmap(Image image) {
        try {
            Image.Plane[] planes = image.getPlanes();
            int width = image.getWidth();
            int height = image.getHeight();

            Log.d(TAG, "Converting Image to grayscale Bitmap: " + width + "x" + height);

            // Get Y plane (luminance) - this is already grayscale!
            ByteBuffer yBuffer = planes[0].getBuffer();
            int ySize = yBuffer.remaining();

            // Create grayscale pixel array
            int[] pixels = new int[width * height];
            byte[] yData = new byte[ySize];
            yBuffer.get(yData);

            // Convert Y values directly to ARGB grayscale
            for (int i = 0; i < pixels.length; i++) {
                int y = yData[i] & 0xFF; // Convert signed byte to unsigned
                // Create grayscale pixel: ARGB format
                pixels[i] = 0xFF000000 | (y << 16) | (y << 8) | y;
            }

            // Create Bitmap directly from pixel array
            Bitmap bitmap = Bitmap.createBitmap(pixels, width, height, Bitmap.Config.ARGB_8888);

            if (bitmap != null) {
                Log.d(TAG, "Successfully converted Image to grayscale Bitmap: " + bitmap.getWidth() + "x" + bitmap.getHeight());
            }

            return bitmap;

        } catch (Exception e) {
            Log.e(TAG, "Error converting Image to grayscale Bitmap: " + e.getMessage(), e);
            return null;
        }
    }

}
