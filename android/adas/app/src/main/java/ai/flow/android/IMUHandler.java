package ai.flow.android;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.util.Log;
import java.util.Arrays;
import ai.flow.android.Messages.ZMQMessage;
import ai.flow.android.ProtoUtils;


public class IMUHandler implements SensorEventListener {

    private static final String TAG = "IMUHandler";
    private static final int SENSOR_DELAY = SensorManager.SENSOR_DELAY_GAME; // ~50Hz - more reasonable for IMU

    private Context androidContext;
    private SensorManager sensorManager;
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
                break;

            case Sensor.TYPE_GYROSCOPE:
                System.arraycopy(event.values, 0, gyroscopeData, 0, 3);
                // Apply calibration if available
                if (isCalibrated) {
                    for (int i = 0; i < 3; i++) {
                        gyroscopeData[i] -= gyroscopeBias[i];
                    }
                }
                break;

            case Sensor.TYPE_MAGNETIC_FIELD:
                System.arraycopy(event.values, 0, magnetometerData, 0, 3);
                // Apply calibration if available
                if (isCalibrated) {
                    for (int i = 0; i < 3; i++) {
                        magnetometerData[i] -= magnetometerBias[i];
                    }
                }
                break;
        }


        // Send combined IMU data every 10 samples
        if (sampleCount % 10 == 0) {
            Log.d(TAG, "Logging combined IMU data: accel=[" + accelerometerData[0] +
                  "," + accelerometerData[1] + "," + accelerometerData[2] +
                  "], gyro=[" + gyroscopeData[0] + "," + gyroscopeData[1] +
                  "," + gyroscopeData[2] + "], mag=[" + magnetometerData[0] +
                  "," + magnetometerData[1] + "," + magnetometerData[2] + "]");

            long currentTime = System.currentTimeMillis();
            
            // Логируем только если Logger запущен
            if (Logger.getInstance().isRunning()) {
                // Логируем в текстовый файл (как раньше)
                // Logger.getInstance().logIMU(
                //     currentTime,
                //     accelerometerData[0], accelerometerData[1], accelerometerData[2],
                //     gyroscopeData[0], gyroscopeData[1], gyroscopeData[2],
                //     magnetometerData[0], magnetometerData[1], magnetometerData[2]
                // );

                // Создаем protobuf сообщение для bag логирования
                try {
                    // Создаем векторы для protobuf
                    java.util.List<Float> accelList = java.util.Arrays.asList(
                        accelerometerData[0], accelerometerData[1], accelerometerData[2]);
                    java.util.List<Float> gyroList = java.util.Arrays.asList(
                        gyroscopeData[0], gyroscopeData[1], gyroscopeData[2]);
                    java.util.List<Float> magList = java.util.Arrays.asList(
                        magnetometerData[0], magnetometerData[1], magnetometerData[2]);

                    // Создаем ZMQMessage с IMU данными
                    ZMQMessage imuMessage = ProtoUtils.createIMUDataMessage(
                        accelList, gyroList, magList, currentTime);
                    imuMessage = imuMessage.toBuilder().setTopic("sensors/imu").build();

                    // Логируем в bag файл
                    Logger.getInstance().logZMQMessage(imuMessage);
                    
                    Log.d(TAG, "IMU data logged to both text file and bag file");
                } catch (Exception e) {
                    Log.e(TAG, "Error creating IMU protobuf message for bag logging", e);
                }
            } else {
                Log.d(TAG, "Logger not running, skipping IMU data logging");
            }
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        Log.d(TAG, "Sensor " + sensor.getName() + " accuracy changed: " + accuracy);
    }

    private void sendIMUData() {
        try {
            // Use microseconds timestamp for higher precision

            // Optional: Send via ZMQ only if needed for debugging/visualization
            // Uncomment if you need ZMQ transmission:

            // long timestamp = System.currentTimeMillis();
            // ZMQMessage imuMessage = ProtobufUtils.createIMUDataMessage(
            //     accelerometerData, gyroscopeData, magnetometerData, timestamp);

            // // Serialize and publish via ZMQ
            // byte[] imuData = ProtobufUtils.serializeMessage(imuMessage);
            // if (pubHandler != null) {
            //     pubHandler.publish("sensors/imu", imuData);
            // } else {
            //     Log.w(TAG, "Publisher not ready, skipping IMU data publish");
            // }

            // Log.d(TAG, "IMU data sent via ZMQ, size: " + imuData.length + " bytes");

            // // Log message info
            // ProtobufUtils.logMessageInfo(imuMessage);

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

    // Getters for current IMU data
    public boolean isRunning() { return isRunning; }
    public boolean isCalibrated() { return isCalibrated; }
}
