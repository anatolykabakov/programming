#include "utils/file_logger.h"
#include <sys/stat.h>
#include <unistd.h>
#include <cstdarg>
#include <cerrno>
#include "utils/logger.h"

// Helper function for variadic logging
void log_to_file(const char* level, const char* tag, const char* format, ...)
{
  char buffer[4096];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  FileLogger::getInstance().log(level, tag, buffer);
}

void FileLogger::init(const std::string& log_dir)
{
  std::lock_guard<std::mutex> lock(mutex_);

  log_dir_ = log_dir;

  // Check if directory exists
  struct stat st;
  if (stat(log_dir.c_str(), &st) != 0) {
    // Directory doesn't exist, create it
    LOGI("Creating log directory: %s", log_dir.c_str());

    if (mkdir(log_dir.c_str(), 0777) != 0) {
      LOGE("Failed to create directory: %s (errno=%d)", log_dir.c_str(), errno);
      return;
    }

    LOGI("Log directory created successfully");
  } else {
    LOGI("Log directory exists: %s", log_dir.c_str());
  }

  // Open log file
  std::string log_path = getLogFileName();
  log_file_.open(log_path, std::ios::out | std::ios::app);

  if (log_file_.is_open()) {
    initialized_ = true;
    log_file_ << "\n========================================\n";
    log_file_ << "ADAS Log Started: " << getTimestamp() << "\n";
    log_file_ << "========================================\n";
    log_file_.flush();

    LOGI("File logging initialized: %s", log_path.c_str());
  } else {
    LOGE("Failed to open log file: %s (errno=%d)", log_path.c_str(), errno);
  }
}

void FileLogger::log(const std::string& level, const std::string& tag, const std::string& message)
{
  if (!initialized_ || !log_file_.is_open()) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  std::string log_entry = getTimestamp() + " [" + level + "] " + tag + ": " + message + "\n";
  log_file_ << log_entry;
  log_file_.flush();

  current_file_size_ += log_entry.size();
  rotateLogIfNeeded();
}

void FileLogger::close()
{
  std::lock_guard<std::mutex> lock(mutex_);

  if (log_file_.is_open()) {
    log_file_ << "========================================\n";
    log_file_ << "ADAS Log Closed: " << getTimestamp() << "\n";
    log_file_ << "========================================\n";
    log_file_.close();
    initialized_ = false;
  }
}

std::string FileLogger::getTimestamp()
{
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
  ss << "." << std::setfill('0') << std::setw(3) << ms.count();

  return ss.str();
}

std::string FileLogger::getLogFileName()
{
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << log_dir_ << "/adas_" << std::put_time(std::localtime(&time_t_now), "%Y%m%d_%H%M%S") << ".log";

  return ss.str();
}

void FileLogger::rotateLogIfNeeded()
{
  if (current_file_size_ > MAX_FILE_SIZE) {
    log_file_.close();
    std::string new_log_path = getLogFileName();
    log_file_.open(new_log_path, std::ios::out | std::ios::app);
    current_file_size_ = 0;

    if (log_file_.is_open()) {
      log_file_ << "Log rotated from previous file\n";
      log_file_.flush();
    }
  }
}
