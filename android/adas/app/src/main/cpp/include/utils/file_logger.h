#pragma once

#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

// File logger for ADAS logs (writes to /sdcard/adas_logs/)
class FileLogger {
public:
  static FileLogger& getInstance()
  {
    static FileLogger instance;
    return instance;
  }

  void init(const std::string& log_dir = "/sdcard/adas_logs");
  void log(const std::string& level, const std::string& tag, const std::string& message);
  void close();

  FileLogger(const FileLogger&) = delete;
  FileLogger& operator=(const FileLogger&) = delete;

private:
  FileLogger() = default;
  ~FileLogger() { close(); }

  std::string getTimestamp();
  std::string getLogFileName();
  void rotateLogIfNeeded();

  std::ofstream log_file_;
  std::mutex mutex_;
  std::string log_dir_;
  size_t current_file_size_ = 0;
  const size_t MAX_FILE_SIZE = 10 * 1024 * 1024;  // 10 MB
  bool initialized_ = false;
};
