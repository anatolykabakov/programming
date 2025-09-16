#include <jni.h>
#include "adas_app.h"
#include "utils/logger.h"

// Static instance
static std::unique_ptr<AdasApp> adas_app;

// JNI implementations
extern "C" {

JNIEXPORT void JNICALL Java_ai_flow_android_AdasAppHandler_nativeStart(JNIEnv* env, jclass cls, jint fd)
{
  LOGI("JNI nativeStart called with fd: %d", fd);
  try {
    if (!adas_app) {
      LOGI("Creating new AdasApp instance with USB file descriptor");
      adas_app = std::make_unique<AdasApp>(fd);
      LOGI("AdasApp instance created successfully");
    } else {
      LOGI("Using existing AdasApp instance");
    }

    LOGI("Calling adas_app->start()");
    bool result = adas_app->start();
    LOGI("adas_app->start() returned: %s", result ? "true" : "false");

    if (result) {
      LOGI("AdasApp started successfully with USB device via JNI");
    } else {
      LOGE("AdasApp failed to start with USB device via JNI");
    }
  } catch (const std::exception& e) {
    LOGE("Exception in JNI nativeStart: %s", e.what());
  } catch (...) {
    LOGE("Unknown exception in JNI nativeStart");
  }

  LOGI("JNI nativeStart completed");
}

JNIEXPORT void JNICALL Java_ai_flow_android_AdasAppHandler_nativeStop(JNIEnv* env, jclass cls)
{
  LOGI("JNI nativeStop called");
  if (adas_app) {
    adas_app->stop();
    adas_app.reset();
    LOGI("AdasApp stopped and reset via JNI");
  } else {
    LOGI("No AdasApp instance to stop");
  }
  LOGI("JNI nativeStop completed");
}

}  // extern "C"
