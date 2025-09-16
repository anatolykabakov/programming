package ai.flow.android;

import android.util.Log;
import com.google.protobuf.ByteString;
import com.google.protobuf.InvalidProtocolBufferException;

// Generated protobuf classes (will be available after build)
import ai.flow.android.Camera.CameraImage;
import ai.flow.android.Camera.CameraState;
import ai.flow.android.Gps.GPSLocation;
import ai.flow.android.Gps.GPSData;
import ai.flow.android.Imu.IMUData;
import ai.flow.android.Imu.AccelerometerData;
import ai.flow.android.Imu.GyroscopeData;
import ai.flow.android.Imu.MagnetometerData;
import ai.flow.android.Messages.ZMQMessage;
import ai.flow.android.Messages.MessageCounters;

import java.util.Map;

public class ProtobufUtils {
    private static final String TAG = "ProtobufUtils";
    
    // Create camera image message
    public static ZMQMessage createCameraImageMessage(byte[] imageData, int width, int height, 
                                                     String format, int frameId, long timestamp) {
        CameraImage cameraImage = CameraImage.newBuilder()
                .setTimestamp(timestamp)
                .setWidth(width)
                .setHeight(height)
                .setFormat(format)
                .setFrameId(frameId)
                .setImageData(ByteString.copyFrom(imageData))
                .setCameraType(CameraImage.CameraType.WIDE_ROAD)
                .build();
        
        return ZMQMessage.newBuilder()
                .setTimestamp(timestamp)
                .setTopic("wideRoadCameraBuffer")
                .setCameraImage(cameraImage)
                .build();
    }
    
    // Create camera state message
    public static ZMQMessage createCameraStateMessage(int frameId, boolean isRecording, 
                                                     boolean isConnected, long timestamp) {
        CameraState cameraState = CameraState.newBuilder()
                .setTimestamp(timestamp)
                .setFrameId(frameId)
                .setIsRecording(isRecording)
                .setIsConnected(isConnected)
                .setWidth(640)
                .setHeight(480)
                .setFormat("YUV420")
                .build();
        
        return ZMQMessage.newBuilder()
                .setTimestamp(timestamp)
                .setTopic("wideRoadCameraState")
                .setCameraState(cameraState)
                .build();
    }
    
    // Create GPS location message
    public static ZMQMessage createGPSLocationMessage(double latitude, double longitude, 
                                                     double altitude, float speed, float bearing, 
                                                     long timestamp) {
        GPSLocation gpsLocation = GPSLocation.newBuilder()
                .setTimestamp(timestamp)
                .setLatitude(latitude)
                .setLongitude(longitude)
                .setAltitude(altitude)
                .setSpeed(speed)
                .setBearing(bearing)
                .setFixType(GPSLocation.FixType.FIX_3D)
                .build();
        
        return ZMQMessage.newBuilder()
                .setTimestamp(timestamp)
                .setTopic("gpsLocation")
                .setGpsLocation(gpsLocation)
                .build();
    }
    
    // Create GPS data message
    public static ZMQMessage createGPSDataMessage(double latitude, double longitude, 
                                                  double altitude, float speed, float bearing, 
                                                  long timestamp) {
        GPSData gpsData = GPSData.newBuilder()
                .setTimestamp(timestamp)
                .setLatitude(latitude)
                .setLongitude(longitude)
                .setAltitude(altitude)
                .setSpeed(speed)
                .setBearing(bearing)
                .setGpsTimestamp(timestamp)
                .setProvider("gps")
                .build();
        
        return ZMQMessage.newBuilder()
                .setTimestamp(timestamp)
                .setTopic("gpsData")
                .setGpsData(gpsData)
                .build();
    }
    
    // Create IMU data message
    public static ZMQMessage createIMUDataMessage(float[] accel, float[] gyro, float[] mag, 
                                                  long timestamp) {
        IMUData imuData = IMUData.newBuilder()
                .setTimestamp(timestamp)
                .setAccelX(accel[0])
                .setAccelY(accel[1])
                .setAccelZ(accel[2])
                .setGyroX(gyro[0])
                .setGyroY(gyro[1])
                .setGyroZ(gyro[2])
                .setMagX(mag[0])
                .setMagY(mag[1])
                .setMagZ(mag[2])
                .setSampleCount(1)
                .build();
        
        return ZMQMessage.newBuilder()
                .setTimestamp(timestamp)
                .setTopic("imuData")
                .setImuData(imuData)
                .build();
    }
    
    // Create message counters
    public static ZMQMessage createMessageCountersMessage(Map<String, Integer> counters, 
                                                          Map<String, Float> rates, long timestamp) {
        MessageCounters.Builder countersBuilder = MessageCounters.newBuilder()
                .setTimestamp(timestamp);
        
        // Add counters
        for (Map.Entry<String, Integer> entry : counters.entrySet()) {
            countersBuilder.putCounters(entry.getKey(), entry.getValue());
        }
        
        // Add rates
        for (Map.Entry<String, Float> entry : rates.entrySet()) {
            countersBuilder.putRates(entry.getKey(), entry.getValue());
        }
        
        return ZMQMessage.newBuilder()
                .setTimestamp(timestamp)
                .setTopic("messageCounters")
                .build();
    }
    
    // Serialize ZMQ message to bytes
    public static byte[] serializeMessage(ZMQMessage message) {
        return message.toByteArray();
    }
    
    // Deserialize bytes to ZMQ message
    public static ZMQMessage deserializeMessage(byte[] data) {
        try {
            return ZMQMessage.parseFrom(data);
        } catch (InvalidProtocolBufferException e) {
            Log.e(TAG, "Error deserializing ZMQ message", e);
            return null;
        }
    }
    
    // Get message size
    public static int getMessageSize(ZMQMessage message) {
        return message.getSerializedSize();
    }
    
    // Log message info
    public static void logMessageInfo(ZMQMessage message) {
        Log.d(TAG, String.format("Message: topic=%s, timestamp=%d, size=%d bytes",
                message.getTopic(), message.getTimestamp(), message.getSerializedSize()));
    }
    
    // Create accelerometer data message
    public static ZMQMessage createAccelerometerDataMessage(float[] accel, int accuracy, 
                                                           boolean isCalibrated, float[] bias, 
                                                           int sampleCount, long timestamp) {
        AccelerometerData.Builder accelBuilder = AccelerometerData.newBuilder()
                .setTimestamp(timestamp)
                .setX(accel[0])
                .setY(accel[1])
                .setZ(accel[2])
                .setAccuracy(accuracy)
                .setSampleCount(sampleCount)
                .setIsCalibrated(isCalibrated);
        
        if (isCalibrated && bias != null) {
            accelBuilder.setBiasX(bias[0])
                       .setBiasY(bias[1])
                       .setBiasZ(bias[2]);
        }
        
        AccelerometerData accelerometerData = accelBuilder.build();
        
        return ZMQMessage.newBuilder()
                .setTimestamp(timestamp)
                .setTopic("accelerometerData")
                .setAccelerometerData(accelerometerData)
                .build();
    }
    
    // Create gyroscope data message
    public static ZMQMessage createGyroscopeDataMessage(float[] gyro, int accuracy, 
                                                       boolean isCalibrated, float[] bias, 
                                                       int sampleCount, long timestamp) {
        GyroscopeData.Builder gyroBuilder = GyroscopeData.newBuilder()
                .setTimestamp(timestamp)
                .setX(gyro[0])
                .setY(gyro[1])
                .setZ(gyro[2])
                .setAccuracy(accuracy)
                .setSampleCount(sampleCount)
                .setIsCalibrated(isCalibrated);
        
        if (isCalibrated && bias != null) {
            gyroBuilder.setBiasX(bias[0])
                      .setBiasY(bias[1])
                      .setBiasZ(bias[2]);
        }
        
        GyroscopeData gyroscopeData = gyroBuilder.build();
        
        return ZMQMessage.newBuilder()
                .setTimestamp(timestamp)
                .setTopic("gyroscopeData")
                .setGyroscopeData(gyroscopeData)
                .build();
    }
    
    // Create magnetometer data message
    public static ZMQMessage createMagnetometerDataMessage(float[] mag, int accuracy, 
                                                          boolean isCalibrated, float[] bias, 
                                                          float fieldStrength, int sampleCount, 
                                                          long timestamp) {
        MagnetometerData.Builder magBuilder = MagnetometerData.newBuilder()
                .setTimestamp(timestamp)
                .setX(mag[0])
                .setY(mag[1])
                .setZ(mag[2])
                .setAccuracy(accuracy)
                .setSampleCount(sampleCount)
                .setIsCalibrated(isCalibrated)
                .setFieldStrength(fieldStrength);
        
        if (isCalibrated && bias != null) {
            magBuilder.setBiasX(bias[0])
                     .setBiasY(bias[1])
                     .setBiasZ(bias[2]);
        }
        
        MagnetometerData magnetometerData = magBuilder.build();
        
        return ZMQMessage.newBuilder()
                .setTimestamp(timestamp)
                .setTopic("magnetometerData")
                .setMagnetometerData(magnetometerData)
                .build();
    }
    
}
