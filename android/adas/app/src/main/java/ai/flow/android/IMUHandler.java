package ai.flow.android;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.util.Log;

import java.nio.ByteBuffer;
import java.util.Arrays;

import ai.flow.android.Messages.ZMQMessage;
import messaging.ZMQPubHandler;

public class IMUHandler implements SensorEventListener {
    
    private static final String TAG = "IMUHandler";
    private static final int SENSOR_DELAY = SensorManager.SENSOR_DELAY_GAME; // ~50Hz - more reasonable for IMU
    
    private Context androidContext;
    private SensorManager sensorManager;
    private ZMQPubHandler pubHandler;
    private boolean isRunning = false;
    
    // Sensors
    private Sensor accelerometer;
    private Sensor gyroscope;
    private Sensor magnetometer;
    
    // IMU data
    private float[] accelerometerData = new float[3];
    private float[] gyroscopeData = new float[3];
    private float[] magnetometerData = new float[3];
    private long timestamp = 0;
    private int sampleCount = 0;
    
    // Calibration data
    private float[] accelerometerBias = new float[3];
    private float[] gyroscopeBias = new float[3];
    private float[] magnetometerBias = new float[3];
    private boolean isCalibrated = false;
    
    public IMUHandler(Context context) {
        this.androidContext = context;
        this.sensorManager = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
        this.pubHandler = new ZMQPubHandler(context);
        
        // Initialize ZMQ publishers
        boolean zmqInit = pubHandler.createPublishers(Arrays.asList(
            "imuData", 
            "accelerometerData", 
            "gyroscopeData", 
            "magnetometerData"
        ));
        if (!zmqInit) {
            Log.e(TAG, "Failed to initialize ZMQ publishers");
        }
        
        // Get sensor references
        if (sensorManager != null) {
            accelerometer = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
            gyroscope = sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
            magnetometer = sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
        }
    }
    
    public void start() {
        if (isRunning) {
            Log.w(TAG, "IMU handler already running");
            return;
        }
        
        if (sensorManager == null) {
            Log.e(TAG, "Sensor manager not available");
            return;
        }
        
        try {
            // Register listeners for all sensors
            if (accelerometer != null) {
                sensorManager.registerListener(this, accelerometer, SENSOR_DELAY);
                Log.d(TAG, "Accelerometer registered");
            } else {
                Log.w(TAG, "Accelerometer not available");
            }
            
            if (gyroscope != null) {
                sensorManager.registerListener(this, gyroscope, SENSOR_DELAY);
                Log.d(TAG, "Gyroscope registered");
            } else {
                Log.w(TAG, "Gyroscope not available");
            }
            
            if (magnetometer != null) {
                sensorManager.registerListener(this, magnetometer, SENSOR_DELAY);
                Log.d(TAG, "Magnetometer registered");
            } else {
                Log.w(TAG, "Magnetometer not available");
            }
            
            isRunning = true;
            Log.i(TAG, "IMU handler started");
            
        } catch (SecurityException e) {
            Log.e(TAG, "SecurityException: Permission denied for high sampling rate sensors. " +
                  "Please grant HIGH_SAMPLING_RATE_SENSORS permission in app settings.", e);
            // Try with slower sampling rate as fallback
            try {
                Log.i(TAG, "Trying with slower sampling rate as fallback...");
                if (accelerometer != null) {
                    sensorManager.registerListener(this, accelerometer, SensorManager.SENSOR_DELAY_NORMAL);
                }
                if (gyroscope != null) {
                    sensorManager.registerListener(this, gyroscope, SensorManager.SENSOR_DELAY_NORMAL);
                }
                if (magnetometer != null) {
                    sensorManager.registerListener(this, magnetometer, SensorManager.SENSOR_DELAY_NORMAL);
                }
                isRunning = true;
                Log.i(TAG, "IMU handler started with fallback sampling rate");
            } catch (Exception fallbackException) {
                Log.e(TAG, "Failed to start IMU handler even with fallback sampling rate", fallbackException);
            }
        } catch (Exception e) {
            Log.e(TAG, "Error starting IMU handler", e);
        }
    }
    
    public void stop() {
        if (!isRunning) {
            return;
        }
        
        if (sensorManager != null) {
            sensorManager.unregisterListener(this);
        }
        
        isRunning = false;
        Log.i(TAG, "IMU handler stopped");
    }
    
    public void dispose() {
        stop();
        if (pubHandler != null) {
            pubHandler.releaseAll();
        }
    }
    
    @Override
    public void onSensorChanged(SensorEvent event) {
        if (event == null || event.values == null) {
            return;
        }
        
        timestamp = event.timestamp;
        sampleCount++;
        
        // Update sensor data based on type
        switch (event.sensor.getType()) {
            case Sensor.TYPE_ACCELEROMETER:
                System.arraycopy(event.values, 0, accelerometerData, 0, 3);
                // Apply calibration if available
                if (isCalibrated) {
                    for (int i = 0; i < 3; i++) {
                        accelerometerData[i] -= accelerometerBias[i];
                    }
                }
                // sendAccelerometerData();
                break;
                
            case Sensor.TYPE_GYROSCOPE:
                System.arraycopy(event.values, 0, gyroscopeData, 0, 3);
                // Apply calibration if available
                if (isCalibrated) {
                    for (int i = 0; i < 3; i++) {
                        gyroscopeData[i] -= gyroscopeBias[i];
                    }
                }
                // sendGyroscopeData();
                break;
                
            case Sensor.TYPE_MAGNETIC_FIELD:
                System.arraycopy(event.values, 0, magnetometerData, 0, 3);
                // Apply calibration if available
                if (isCalibrated) {
                    for (int i = 0; i < 3; i++) {
                        magnetometerData[i] -= magnetometerBias[i];
                    }
                }
                // sendMagnetometerData();
                break;
        }
        
        // Send individual sensor data every 5 samples
        if (sampleCount % 5 == 0) {
            sendAccelerometerData();
        }
        if (sampleCount % 5 == 1) {
            sendGyroscopeData();
        }
        if (sampleCount % 5 == 2) {
            sendMagnetometerData();
        }
        
        // Send combined IMU data every 10 samples
        if (sampleCount % 10 == 0) {
            sendIMUData();
        }
    }
    
    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        Log.d(TAG, "Sensor " + sensor.getName() + " accuracy changed: " + accuracy);
    }
    
    private void sendIMUData() {
        try {
            long timestamp = System.currentTimeMillis();
            
            Log.d(TAG, "Sending combined IMU data: accel=[" + accelerometerData[0] + 
                  "," + accelerometerData[1] + "," + accelerometerData[2] + 
                  "], gyro=[" + gyroscopeData[0] + "," + gyroscopeData[1] + 
                  "," + gyroscopeData[2] + "], mag=[" + magnetometerData[0] + 
                  "," + magnetometerData[1] + "," + magnetometerData[2] + "]");
            
            // Create protobuf IMU data message
            ZMQMessage imuMessage = ProtobufUtils.createIMUDataMessage(
                accelerometerData, gyroscopeData, magnetometerData, timestamp);
            
            // Serialize and publish via ZMQ
            byte[] imuData = ProtobufUtils.serializeMessage(imuMessage);
            if (pubHandler != null) {
                pubHandler.publish("imuData", imuData);
            } else {
                Log.w(TAG, "Publisher not ready, skipping IMU data publish");
            }
            
            Log.d(TAG, "IMU data sent via ZMQ, size: " + imuData.length + " bytes");
            
            // Log message info
            ProtobufUtils.logMessageInfo(imuMessage);
            
        } catch (Exception e) {
            Log.e(TAG, "Error sending IMU data", e);
        }
    }

    // Calibration methods
    public void startCalibration() {
        Log.i(TAG, "Starting IMU calibration...");
        isCalibrated = false;
        // Reset bias values
        Arrays.fill(accelerometerBias, 0.0f);
        Arrays.fill(gyroscopeBias, 0.0f);
        Arrays.fill(magnetometerBias, 0.0f);
    }
    
    public void stopCalibration() {
        Log.i(TAG, "IMU calibration completed");
        isCalibrated = true;
    }
    
    // Send individual accelerometer data
    private void sendAccelerometerData() {
        try {
            long timestamp = System.currentTimeMillis();
            
            Log.d(TAG, "Sending accelerometer data: [" + accelerometerData[0] + 
                  "," + accelerometerData[1] + "," + accelerometerData[2] + "]");
            
            // Create protobuf accelerometer data message
            ZMQMessage accelMessage = ProtobufUtils.createAccelerometerDataMessage(
                accelerometerData, 3, isCalibrated, accelerometerBias, sampleCount, timestamp);
            
            // Serialize and publish via ZMQ
            byte[] accelData = ProtobufUtils.serializeMessage(accelMessage);
            if (pubHandler != null) {
                pubHandler.publish("accelerometerData", accelData);
            }
            
            Log.d(TAG, "Accelerometer data sent via ZMQ, size: " + accelData.length + " bytes");
            
        } catch (Exception e) {
            Log.e(TAG, "Error sending accelerometer data", e);
        }
    }
    
    // Send individual gyroscope data
    private void sendGyroscopeData() {
        try {
            long timestamp = System.currentTimeMillis();
            
            Log.d(TAG, "Sending gyroscope data: [" + gyroscopeData[0] + 
                  "," + gyroscopeData[1] + "," + gyroscopeData[2] + "]");
            
            // Create protobuf gyroscope data message
            ZMQMessage gyroMessage = ProtobufUtils.createGyroscopeDataMessage(
                gyroscopeData, 3, isCalibrated, gyroscopeBias, sampleCount, timestamp);
            
            // Serialize and publish via ZMQ
            byte[] gyroData = ProtobufUtils.serializeMessage(gyroMessage);
            if (pubHandler != null) {
                pubHandler.publish("gyroscopeData", gyroData);
            }
            
            Log.d(TAG, "Gyroscope data sent via ZMQ, size: " + gyroData.length + " bytes");
            
        } catch (Exception e) {
            Log.e(TAG, "Error sending gyroscope data", e);
        }
    }
    
    // Send individual magnetometer data
    private void sendMagnetometerData() {
        try {
            long timestamp = System.currentTimeMillis();
            
            // Calculate magnetic field strength
            float fieldStrength = (float) Math.sqrt(
                magnetometerData[0] * magnetometerData[0] + 
                magnetometerData[1] * magnetometerData[1] + 
                magnetometerData[2] * magnetometerData[2]);
            
            Log.d(TAG, "Sending magnetometer data: [" + magnetometerData[0] + 
                  "," + magnetometerData[1] + "," + magnetometerData[2] + 
                  "], field strength: " + fieldStrength);
            
            // Create protobuf magnetometer data message
            ZMQMessage magMessage = ProtobufUtils.createMagnetometerDataMessage(
                magnetometerData, 3, isCalibrated, magnetometerBias, 
                fieldStrength, sampleCount, timestamp);
            
            // Serialize and publish via ZMQ
            byte[] magData = ProtobufUtils.serializeMessage(magMessage);
            if (pubHandler != null) {
                pubHandler.publish("magnetometerData", magData);
            }
            
            Log.d(TAG, "Magnetometer data sent via ZMQ, size: " + magData.length + " bytes");
            
        } catch (Exception e) {
            Log.e(TAG, "Error sending magnetometer data", e);
        }
    }

    // Getters for current IMU data
    public float[] getAccelerometerData() { return accelerometerData.clone(); }
    public float[] getGyroscopeData() { return gyroscopeData.clone(); }
    public float[] getMagnetometerData() { return magnetometerData.clone(); }
    public long getTimestamp() { return timestamp; }
    public int getSampleCount() { return sampleCount; }
    public boolean isRunning() { return isRunning; }
    public boolean isCalibrated() { return isCalibrated; }
}
