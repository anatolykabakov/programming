package ai.flow.android;

import android.media.Image;
import android.util.Log;
import java.nio.ByteBuffer;

public class Utils {
    private static final String TAG = "Utils";
    
    public static void fillYUVBuffer(Image image, ByteBuffer yuvBuffer) {
        Image.Plane[] planes = image.getPlanes();
        int format = image.getFormat();
        
        Log.d(TAG, "Image format: " + format + ", planes: " + planes.length);
        
        // Reset buffer position to start
        yuvBuffer.rewind();
        
        // Y plane (always present)
        ByteBuffer yPlane = planes[0].getBuffer();
        int ySize = yPlane.remaining();
        yuvBuffer.put(yPlane);
        Log.d(TAG, "Y plane size: " + ySize);
        
        // Handle UV planes based on format
        if (planes.length > 1 && planes[1] != null) {
            // Check if UV planes are interleaved (NV12/NV21 format)
            if (planes[1].getPixelStride() == 2) {
                // Interleaved UV pixels (NV12 or NV21)
                if (planes.length > 2 && planes[2] != null) {
                    ByteBuffer uvPlane = planes[2].getBuffer();
                    int uvSize = uvPlane.remaining();
                    yuvBuffer.put(uvPlane);
                    Log.d(TAG, "UV plane size (interleaved): " + uvSize);
                }
            } else {
                // Separate U and V planes
                ByteBuffer uPlane = planes[1].getBuffer();
                int uSize = uPlane.remaining();
                yuvBuffer.put(uPlane);
                Log.d(TAG, "U plane size: " + uSize);
                
                if (planes.length > 2 && planes[2] != null) {
                    ByteBuffer vPlane = planes[2].getBuffer();
                    int vSize = vPlane.remaining();
                    yuvBuffer.put(vPlane);
                    Log.d(TAG, "V plane size: " + vSize);
                }
            }
        } else {
            Log.w(TAG, "UV planes not available");
        }
        
        Log.d(TAG, "Total YUV buffer size: " + yuvBuffer.position() + ", capacity: " + yuvBuffer.capacity());
    }
}

