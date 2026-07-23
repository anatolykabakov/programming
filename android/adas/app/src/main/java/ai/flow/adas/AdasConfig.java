package ai.flow.adas;

import android.content.Context;
import android.util.Log;

import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

/** Reads Java-needed bits from {@code assets/config.json} / filesDir override. */
public final class AdasConfig {
    private static final String TAG = "AdasConfig";
    public static final String ASSET = "config.json";
    private static final String DEFAULT_MODEL = "supercombo.onnx";

    private AdasConfig() {}

    /** Prefer filesDir override; on corrupt override fall back to asset. */
    private static JSONObject root(Context context) throws Exception {
        File file = RuntimeParams.configFile(context);
        if (file.exists() && file.length() > 0) {
            try {
                return parseStream(new FileInputStream(file));
            } catch (Exception e) {
                Log.w(TAG, "Corrupt override — falling back to asset", e);
            }
        }
        return parseStream(context.getAssets().open(ASSET));
    }

    private static JSONObject parseStream(InputStream in) throws Exception {
        try (BufferedReader br = new BufferedReader(new InputStreamReader(in, StandardCharsets.UTF_8))) {
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = br.readLine()) != null) {
                sb.append(line).append('\n');
            }
            return new JSONObject(sb.toString());
        }
    }

    /** Supercombo ONNX asset path under assets/ (or sdcard override name). */
    public static String supercomboAsset(Context context) {
        try {
            String name = root(context).optString("supercombo_asset", DEFAULT_MODEL);
            if (name == null || name.isEmpty()) {
                name = DEFAULT_MODEL;
            }
            Log.i(TAG, "supercombo_asset=" + name);
            return name;
        } catch (Exception e) {
            Log.w(TAG, "Failed to read " + ASSET + ", using " + DEFAULT_MODEL, e);
            return DEFAULT_MODEL;
        }
    }

    /** nodes.vision_supercombo — when false, skip ONNX VisionPipeline. */
    public static boolean visionSupercomboEnabled(Context context) {
        try {
            JSONObject nodes = root(context).optJSONObject("nodes");
            if (nodes == null) {
                return true;
            }
            return nodes.optBoolean("vision_supercombo", true);
        } catch (Exception e) {
            return true;
        }
    }
}
