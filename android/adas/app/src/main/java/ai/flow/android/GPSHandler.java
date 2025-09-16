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

import java.nio.ByteBuffer;
import java.util.Arrays;

import ai.flow.android.Messages.ZMQMessage;
import messaging.ZMQPubHandler;

public class GPSHandler implements LocationListener {
    
    private static final String TAG = "GPSHandler";
    private static final long MIN_TIME_MS = 1000; // 1 second
    private static final float MIN_DISTANCE_M = 1.0f; // 1 meter
    
    private Context context;
    private LocationManager locationManager;
    private ZMQPubHandler pubHandler;
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
        this.pubHandler = new ZMQPubHandler(context);
        
        // Initialize ZMQ publishers
        boolean zmqInit = pubHandler.createPublishers(Arrays.asList("gpsLocation", "gpsData"));
        if (!zmqInit) {
            Log.e(TAG, "Failed to initialize ZMQ publishers");
        }
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
            // Request location updates from GPS provider
            locationManager.requestLocationUpdates(
                LocationManager.GPS_PROVIDER,
                MIN_TIME_MS,
                MIN_DISTANCE_M,
                this
            );
            
            // Also try network provider as fallback
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
        if (pubHandler != null) {
            pubHandler.releaseAll();
        }
    }
    
    @Override
    public void onLocationChanged(Location location) {
        if (location == null) {
            return;
        }
        
        // Update GPS data
        latitude = location.getLatitude();
        longitude = location.getLongitude();
        altitude = location.getAltitude();
        speed = location.getSpeed();
        bearing = location.getBearing();
        accuracy = location.getAccuracy();
        timestamp = location.getTime();
        
        Log.d(TAG, String.format("GPS: lat=%.6f, lon=%.6f, alt=%.1f, speed=%.1f, accuracy=%.1f",
            latitude, longitude, altitude, speed, accuracy));
        
        // Send via ZMQ
        sendGPSData();
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
            long timestamp = System.currentTimeMillis();
            
            Log.d(TAG, "Sending GPS data: lat=" + latitude + ", lon=" + longitude + 
                  ", alt=" + altitude + ", speed=" + speed + ", bearing=" + bearing);
            
            // Create protobuf GPS location message
            ZMQMessage locationMessage = ProtobufUtils.createGPSLocationMessage(
                latitude, longitude, altitude, speed, bearing, timestamp);
            
            // Create protobuf GPS data message
            ZMQMessage dataMessage = ProtobufUtils.createGPSDataMessage(
                latitude, longitude, altitude, speed, bearing, timestamp);
            
            // Serialize and publish via ZMQ
            byte[] locationData = ProtobufUtils.serializeMessage(locationMessage);
            byte[] gpsData = ProtobufUtils.serializeMessage(dataMessage);
            
            Log.d(TAG, "GPS message sizes: location=" + locationData.length + 
                  " bytes, data=" + gpsData.length + " bytes");
            
            if (pubHandler != null) {
                pubHandler.publish("gpsLocation", locationData);
                pubHandler.publish("gpsData", gpsData);
            } else {
                Log.w(TAG, "Publisher not ready, skipping GPS data publish");
            }
            
            // Log message info
            ProtobufUtils.logMessageInfo(locationMessage);
            
            Log.d(TAG, "GPS data sent via ZMQ successfully");
            
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
