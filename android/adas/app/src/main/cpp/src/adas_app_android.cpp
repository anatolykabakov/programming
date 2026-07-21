#include <jni.h>
#include <string>

#include "adas_app.h"
#include "utils/adas_config.h"
#include "utils/logger.h"

static std::unique_ptr<AdasApp> adas_app;

static std::string jstringToStd(JNIEnv* env, jstring value)
{
  if (!value)
    return {};
  const char* chars = env->GetStringUTFChars(value, nullptr);
  std::string out = chars ? chars : "";
  if (chars)
    env->ReleaseStringUTFChars(value, chars);
  return out;
}

extern "C" {

JNIEXPORT void JNICALL Java_ai_flow_adas_AdasAppHandler_nativeStart(JNIEnv* env, jclass /*cls*/, jint fd,
                                                                    jstring dbcPath, jstring configPath)
{
  const std::string dbc_path = jstringToStd(env, dbcPath);
  const std::string config_path = jstringToStd(env, configPath);

  bool cfg_ok = false;
  adas::AdasRuntimeConfig cfg = adas::loadAdasRuntimeConfig(config_path, &cfg_ok);
  if (!cfg_ok) {
    LOGW("JNI nativeStart: config load failed (%s), continuing with defaults", config_path.c_str());
  }

  LOGI("JNI nativeStart fd=%d dbc=%s config=%s lane_keep=%d loc=%d", fd, dbc_path.c_str(), config_path.c_str(),
       cfg.lane_keep ? 1 : 0, cfg.localization ? 1 : 0);
  try {
    if (!adas_app) {
      adas_app = std::make_unique<AdasApp>(fd, dbc_path, cfg);
    }
    if (!adas_app->start()) {
      LOGE("AdasApp failed to start");
    }
  } catch (const std::exception& e) {
    LOGE("Exception in JNI nativeStart: %s", e.what());
  } catch (...) {
    LOGE("Unknown exception in JNI nativeStart");
  }
}

JNIEXPORT void JNICALL Java_ai_flow_adas_AdasAppHandler_nativeStop(JNIEnv* /*env*/, jclass /*cls*/)
{
  if (adas_app) {
    adas_app->stop();
    adas_app.reset();
  }
}

}  // extern "C"
