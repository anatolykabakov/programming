package ai.flow.adas;

import android.os.SystemClock;

/**
 * Shared monotonic clock for bag sync (camera / lanes / IMU / GPS / vehicle).
 * Matches C++ CLOCK_BOOTTIME used in native getCurrentTimestamp().
 */
public final class TimeUtil {
    private TimeUtil() {}

    /** Milliseconds since boot, including deep sleep (elapsedRealtime). */
    public static long nowMs() {
        return SystemClock.elapsedRealtime();
    }
}
