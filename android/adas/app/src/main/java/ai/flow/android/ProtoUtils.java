package ai.flow.android;

import android.util.Log;
import java.util.List;
import java.util.ArrayList;
import bag.BagOuterClass;
import ai.flow.android.Messages;
import ai.flow.android.Imu;
import ai.flow.android.Gps;
import ai.flow.android.Camera;
import ai.flow.android.CameraIntrinsicsOuterClass;

/**
 * Утилиты для конвертации protobuf сообщений в bag формат
 */
public class ProtoUtils {
    private static final String TAG = "ProtoUtils";

    /**
     * Конвертирует ZMQMessage в bag.Bag сообщение
     */
    public static BagOuterClass.Bag createBagMessage(Messages.ZMQMessage zmqMessage) {
        BagOuterClass.Bag.Builder bagBuilder = BagOuterClass.Bag.newBuilder();
        
        // Устанавливаем timestamp
        com.google.protobuf.Timestamp timestamp = com.google.protobuf.util.Timestamps.fromMillis(zmqMessage.getTimestamp());
        bagBuilder.setTimestamp(timestamp);
        
        // Добавляем сообщение
        bagBuilder.addMessages(zmqMessage);
        
        return bagBuilder.build();
    }

    /**
     * Конвертирует список ZMQMessage в bag.Bag сообщение
     */
    public static BagOuterClass.Bag createBagMessage(List<Messages.ZMQMessage> zmqMessages) {
        BagOuterClass.Bag.Builder bagBuilder = BagOuterClass.Bag.newBuilder();
        
        if (!zmqMessages.isEmpty()) {
            // Устанавливаем timestamp из первого сообщения
            com.google.protobuf.Timestamp timestamp = com.google.protobuf.util.Timestamps.fromMillis(zmqMessages.get(0).getTimestamp());
            bagBuilder.setTimestamp(timestamp);
            
            // Добавляем все сообщения
            bagBuilder.addAllMessages(zmqMessages);
        }
        
        return bagBuilder.build();
    }

    /**
     * Конвертирует topic name в имя директории (заменяет / на __)
     */
    public static String fixTopicName(String topicName) {
        if (topicName == null) return "";
        return topicName.replace("/", "__");
    }

    /**
     * Создает имя bag файла в формате BAG_YYYY_MM_DD_HH_mm_ss
     */
    public static String createBagFileName(long timestamp) {
        java.text.SimpleDateFormat sdf = new java.text.SimpleDateFormat("yyyy_MM_dd_HH_mm_ss", java.util.Locale.US);
        return "BAG_" + sdf.format(new java.util.Date(timestamp));
    }

    /**
     * Создает имя файла с данными в формате YYYY_MM_DD_HH_mm_ss.bin
     */
    public static String createDataFileName(long timestamp) {
        java.text.SimpleDateFormat sdf = new java.text.SimpleDateFormat("yyyy_MM_dd_HH_mm_ss", java.util.Locale.US);
        return sdf.format(new java.util.Date(timestamp)) + ".bin";
    }

    /**
     * Проверяет, является ли сообщение валидным для bag логирования
     */
    public static boolean isValidForBagLogging(Messages.ZMQMessage message) {
        if (message == null) return false;
        
        String topic = message.getTopic();
        if (topic == null || topic.isEmpty()) return false;
        
        // Проверяем, что у сообщения есть payload
        return message.hasCameraImage() || 
               message.hasGpsLocation() || 
               message.hasGpsData() || 
               message.hasImuData() || 
               message.hasCanData() || 
               message.hasPandaHealth();
    }

    /**
     * Получает размер сообщения в байтах
     */
    public static int getMessageSize(Messages.ZMQMessage message) {
        if (message == null) return 0;
        return message.getSerializedSize();
    }

    /**
     * Получает размер bag сообщения в байтах
     */
    public static int getBagSize(BagOuterClass.Bag bag) {
        if (bag == null) return 0;
        return bag.getSerializedSize();
    }

    /**
     * Создает IMU Data сообщение
     */
    public static Messages.ZMQMessage createIMUDataMessage(java.util.List<Float> accel,
                                                          java.util.List<Float> gyro,
                                                          java.util.List<Float> mag, 
                                                          long timestamp) {
        Messages.ZMQMessage zmqMessage = Messages.ZMQMessage.newBuilder()
            .setTimestamp(timestamp)
            .setTopic("sensors/imu")
            .build();

        Imu.IMUData.Builder imuBuilder = Imu.IMUData.newBuilder();
        imuBuilder.setTimestamp(timestamp);

        if (accel != null && accel.size() >= 3) {
            imuBuilder.setAccelX(accel.get(0));
            imuBuilder.setAccelY(accel.get(1));
            imuBuilder.setAccelZ(accel.get(2));
        }

        if (gyro != null && gyro.size() >= 3) {
            imuBuilder.setGyroX(gyro.get(0));
            imuBuilder.setGyroY(gyro.get(1));
            imuBuilder.setGyroZ(gyro.get(2));
        }

        if (mag != null && mag.size() >= 3) {
            imuBuilder.setMagX(mag.get(0));
            imuBuilder.setMagY(mag.get(1));
            imuBuilder.setMagZ(mag.get(2));
        }

        imuBuilder.setSampleCount(1);

        return zmqMessage.toBuilder()
            .setImuData(imuBuilder.build())
            .build();
    }

    /**
     * Создает GPS Location сообщение
     */
    public static Messages.ZMQMessage createGPSLocationMessage(double latitude, double longitude,
                                                              double altitude, float speed, 
                                                              float bearing, long timestamp) {
        Messages.ZMQMessage zmqMessage = Messages.ZMQMessage.newBuilder()
            .setTimestamp(timestamp)
            .setTopic("sensors/gps/location")
            .build();

        Gps.GPSLocation.Builder gpsBuilder = Gps.GPSLocation.newBuilder();
        gpsBuilder.setTimestamp(timestamp);
        gpsBuilder.setLatitude(latitude);
        gpsBuilder.setLongitude(longitude);
        gpsBuilder.setAltitude(altitude);
        gpsBuilder.setSpeed(speed);
        gpsBuilder.setBearing(bearing);
        gpsBuilder.setFixType(Gps.GPSLocation.FixType.FIX_3D);

        return zmqMessage.toBuilder()
            .setGpsLocation(gpsBuilder.build())
            .build();
    }

    /**
     * Создает GPS Data сообщение
     */
    public static Messages.ZMQMessage createGPSDataMessage(double latitude, double longitude,
                                                          double altitude, float speed, 
                                                          float bearing, long timestamp) {
        Messages.ZMQMessage zmqMessage = Messages.ZMQMessage.newBuilder()
            .setTimestamp(timestamp)
            .setTopic("sensors/gps/data")
            .build();

        Gps.GPSData.Builder gpsBuilder = Gps.GPSData.newBuilder();
        gpsBuilder.setTimestamp(timestamp);
        gpsBuilder.setLatitude(latitude);
        gpsBuilder.setLongitude(longitude);
        gpsBuilder.setAltitude(altitude);
        gpsBuilder.setSpeed(speed);
        gpsBuilder.setBearing(bearing);
        gpsBuilder.setGpsTimestamp(timestamp);
        gpsBuilder.setProvider("gps");

        return zmqMessage.toBuilder()
            .setGpsData(gpsBuilder.build())
            .build();
    }

    /**
     * Создает Camera Image сообщение
     */
    public static Messages.ZMQMessage createCameraImageMessage(java.util.List<Byte> imageData, 
                                                              int width, int height, 
                                                              String format, int frameId, 
                                                              long timestamp) {
        Messages.ZMQMessage zmqMessage = Messages.ZMQMessage.newBuilder()
            .setTimestamp(timestamp)
            .setTopic("sensors/camera/image")
            .build();

        Camera.CameraImage.Builder cameraBuilder = Camera.CameraImage.newBuilder();
        cameraBuilder.setTimestamp(timestamp);
        cameraBuilder.setWidth(width);
        cameraBuilder.setHeight(height);
        cameraBuilder.setFormat(format);
        cameraBuilder.setFrameId(frameId);
        
        // Конвертируем List<Byte> в byte[]
        byte[] imageBytes = new byte[imageData.size()];
        for (int i = 0; i < imageData.size(); i++) {
            imageBytes[i] = imageData.get(i);
        }
        cameraBuilder.setImageData(com.google.protobuf.ByteString.copyFrom(imageBytes));
        cameraBuilder.setCameraType(Camera.CameraImage.CameraType.WIDE_ROAD);

        return zmqMessage.toBuilder()
            .setCameraImage(cameraBuilder.build())
            .build();
    }

    /**
     * Создает Camera Intrinsics сообщение
     */
    public static Messages.ZMQMessage createCameraIntrinsicsMessage(
            float physicalFocalLengthMm,
            float sensorWidthMm, float sensorHeightMm,
            int activeArrayWidth, int activeArrayHeight,
            float[] distortionCoefficients,
            float[] intrinsicCalibration,
            float focalLengthPx,
            int captureWidth, int captureHeight,
            String cameraId, String distortionModel,
            long timestamp) {
        
        Messages.ZMQMessage zmqMessage = Messages.ZMQMessage.newBuilder()
            .setTimestamp(timestamp)
            .setTopic("camera/intrinsics")
            .build();

        CameraIntrinsicsOuterClass.CameraIntrinsics.Builder intrinsicsBuilder = CameraIntrinsicsOuterClass.CameraIntrinsics.newBuilder();
        intrinsicsBuilder.setTimestamp(timestamp);
        intrinsicsBuilder.setPhysicalFocalLengthMm(physicalFocalLengthMm);
        intrinsicsBuilder.setSensorWidthMm(sensorWidthMm);
        intrinsicsBuilder.setSensorHeightMm(sensorHeightMm);
        intrinsicsBuilder.setActiveArrayWidth(activeArrayWidth);
        intrinsicsBuilder.setActiveArrayHeight(activeArrayHeight);
        
        if (distortionCoefficients != null) {
            for (float coeff : distortionCoefficients) {
                intrinsicsBuilder.addDistortionCoefficients(coeff);
            }
        }
        
        if (intrinsicCalibration != null) {
            for (float calib : intrinsicCalibration) {
                intrinsicsBuilder.addIntrinsicCalibration(calib);
            }
        }
        
        intrinsicsBuilder.setFocalLengthPx(focalLengthPx);
        intrinsicsBuilder.setCaptureWidth(captureWidth);
        intrinsicsBuilder.setCaptureHeight(captureHeight);
        intrinsicsBuilder.setCameraId(cameraId);
        intrinsicsBuilder.setDistortionModel(distortionModel);

        return zmqMessage.toBuilder()
            .setCameraIntrinsics(intrinsicsBuilder.build())
            .build();
    }
}
