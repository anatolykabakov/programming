#include <iostream>
#include <chrono>
#include <thread>
#include "adas_app.h"
#include "logger.h"


// Linux main function for standalone execution
int main() {
    LOGI("Starting ADAS application for Linux");
    std::unique_ptr<AdasApp> adas_app;

    try {
        adas_app = std::make_unique<AdasApp>();
        bool result = adas_app->start();

        if (result) {
            LOGI("ADAS application started successfully");

            // Keep running until interrupted
            LOGI("Press Ctrl+C to stop the application");
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
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

    return 0;
}
