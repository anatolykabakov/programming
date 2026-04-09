package ai.flow.android;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.util.Log;

import androidx.core.app.ActivityCompat;
import ai.flow.android.Messages.ZMQMessage;
import ai.flow.android.ProtoUtils;


public class GPSHandler implements LocationListener {

    private static final String TAG = "GPSHandler";
    private static final long MIN_TIME_MS = 1000; // 1 second
    private static final float MIN_DISTANCE_M = 1.0f; // 1 meter

    private Context context;
    private LocationManager locationManager;
    private boolean isRunning = false;

    // GPS data
    private double latitude = 0.0;
    private double longitude = 0.0;
    private double altitude = 0.0;
    private float speed = 0.0f;
    private float bearing = 0.0f;
    private float accuracy = 0.0f;
    private long timestamp = 0;

    public GPSHandler(Context context) {
        this.context = context;
        this.locationManager = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
    }

    public void start() {
        if (isRunning) {
            Log.w(TAG, "GPS handler already running");
            return;
        }

        if (locationManager == null) {
            Log.e(TAG, "Location manager not available");
            return;
        }

        // Check permissions
        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED &&
            ActivityCompat.checkSelfPermission(context, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            Log.e(TAG, "Location permissions not granted");
            return;
        }

        try {
            locationManager.requestLocationUpdates(
                LocationManager.GPS_PROVIDER,
                MIN_TIME_MS,
                MIN_DISTANCE_M,
                this
            );

            locationManager.requestLocationUpdates(
                LocationManager.NETWORK_PROVIDER,
                MIN_TIME_MS,
                MIN_DISTANCE_M,
                this
            );

            isRunning = true;
            Log.i(TAG, "GPS handler started");

        } catch (SecurityException e) {
            Log.e(TAG, "Security exception when requesting location updates", e);
        }
    }

    public void stop() {
        if (!isRunning) {
            return;
        }

        if (locationManager != null) {
            locationManager.removeUpdates(this);
        }

        isRunning = false;
        Log.i(TAG, "GPS handler stopped");
    }

    public void dispose() {
        stop();
    }

    @Override
    public void onLocationChanged(Location location) {
        if (location == null) {
            return;
        }

        latitude = location.getLatitude();
        longitude = location.getLongitude();
        altitude = location.getAltitude();
        speed = location.getSpeed();
        bearing = location.getBearing();
        accuracy = location.getAccuracy();
        timestamp = location.getTime();

        long currentTime = System.currentTimeMillis();
        
        // Логируем только если Logger запущен
        if (Logger.getInstance().isRunning()) {
            // Логируем в текстовый файл (как раньше)
            // Logger.getInstance().logGPS(currentTime, latitude, longitude, altitude, speed, bearing, accuracy);

            // Создаем protobuf сообщения для bag логирования
            try {
                Log.d(TAG, "Creating GPS protobuf messages...");
                
                // Создаем GPS Location сообщение
                ZMQMessage locationMessage = ProtoUtils.createGPSLocationMessage(
                    latitude, longitude, altitude, speed, bearing, currentTime);
                locationMessage = locationMessage.toBuilder().setTopic("sensors/gps/location").build();
                Log.d(TAG, "Created GPS Location message: " + locationMessage.getTopic());
                
                // Создаем GPS Data сообщение
                ZMQMessage dataMessage = ProtoUtils.createGPSDataMessage(
                    latitude, longitude, altitude, speed, bearing, currentTime);
                dataMessage = dataMessage.toBuilder().setTopic("sensors/gps/data").build();
                Log.d(TAG, "Created GPS Data message: " + dataMessage.getTopic());

                // Логируем оба сообщения в bag файлы
                Log.d(TAG, "Calling Logger.logGPSMessage for location...");
                Logger.getInstance().logZMQMessage(locationMessage);
                Log.d(TAG, "Calling Logger.logGPSMessage for data...");
                Logger.getInstance().logZMQMessage(dataMessage);
            
                Log.d(TAG, "GPS data logged to both text file and bag files");
            } catch (Exception e) {
                Log.e(TAG, "Error creating GPS protobuf messages for bag logging", e);
            }
        } else {
            Log.d(TAG, "Logger not running, skipping GPS data logging");
        }
    }

    @Override
    public void onStatusChanged(String provider, int status, Bundle extras) {
        Log.d(TAG, "GPS provider " + provider + " status changed: " + status);
    }

    @Override
    public void onProviderEnabled(String provider) {
        Log.d(TAG, "GPS provider " + provider + " enabled");
    }

    @Override
    public void onProviderDisabled(String provider) {
        Log.d(TAG, "GPS provider " + provider + " disabled");
    }

    private void sendGPSData() {
        try {
            // Use microseconds timestamp for consistency with IMU

            // Optional: Send via ZMQ only if needed for debugging/visualization
            // Uncomment if you need ZMQ transmission:

            // long timestamp = System.currentTimeMillis();
            // ZMQMessage locationMessage = ProtobufUtils.createGPSLocationMessage(
            //     latitude, longitude, altitude, speed, bearing, timestamp);

            // // Create protobuf GPS data message
            // ZMQMessage dataMessage = ProtobufUtils.createGPSDataMessage(
            //     latitude, longitude, altitude, speed, bearing, timestamp);

            // // Serialize and publish via ZMQ
            // byte[] locationData = ProtobufUtils.serializeMessage(locationMessage);
            // byte[] gpsData = ProtobufUtils.serializeMessage(dataMessage);

            // Log.d(TAG, "GPS message sizes: location=" + locationData.length +
            //       " bytes, data=" + gpsData.length + " bytes");

            // if (pubHandler != null) {
            //     pubHandler.publish("sensors/gps/location", locationData);
            //     pubHandler.publish("sensors/gps/data", gpsData);
            // } else {
            //     Log.w(TAG, "Publisher not ready, skipping GPS data publish");
            // }

            // // Log message info
            // ProtobufUtils.logMessageInfo(locationMessage);

            // Log.d(TAG, "GPS data sent via ZMQ successfully");

        } catch (Exception e) {
            Log.e(TAG, "Error sending GPS data via ZMQ", e);
        }
    }

    // Getters for current GPS data
    public double getLatitude() { return latitude; }
    public double getLongitude() { return longitude; }
    public double getAltitude() { return altitude; }
    public float getSpeed() { return speed; }
    public float getBearing() { return bearing; }
    public float getAccuracy() { return accuracy; }
    public long getTimestamp() { return timestamp; }
    public boolean isRunning() { return isRunning; }
}
