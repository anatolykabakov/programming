#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <signal.h>
#include "adas_app.h"
#include "utils/logger.h"

// Global flag for graceful shutdown
std::atomic<bool> g_shutdown_requested{false};
std::unique_ptr<AdasApp> g_adas_app{nullptr};

// Signal handler for graceful shutdown
void signalHandler(int signal)
{
  LOGI("Received signal %d, initiating graceful shutdown...", signal);
  g_shutdown_requested.store(true);
}

int main()
{
  LOGI("Starting ADAS application for Linux");

  // Setup signal handlers for graceful shutdown
  signal(SIGINT, signalHandler);   // Ctrl+C
  signal(SIGTERM, signalHandler);  // Termination signal

  try {
    g_adas_app = std::make_unique<AdasApp>();
    bool result = g_adas_app->start();

    if (result) {
      LOGI("ADAS application started successfully");
      LOGI("Press Ctrl+C to stop the application gracefully");

      // Keep running until shutdown is requested
      while (!g_shutdown_requested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      LOGI("Graceful shutdown initiated...");

    } else {
      LOGE("Failed to start ADAS application");
      return 1;
    }
  } catch (const std::exception& e) {
    LOGE("Exception in main(): %s", e.what());
    return 1;
  } catch (...) {
    LOGE("Unknown exception in main()");
    return 1;
  }

  // Graceful shutdown
  LOGI("Stopping ADAS application...");
  if (g_adas_app) {
    g_adas_app->stop();
    g_adas_app.reset();
  }

  LOGI("ADAS application stopped gracefully");
  return 0;
}
