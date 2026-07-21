#include "utils/adas_config.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>

#include "utils/logger.h"

namespace adas {
namespace {

std::string readFile(const std::string& path, bool* ok)
{
  std::ifstream in(path);
  if (!in) {
    if (ok)
      *ok = false;
    return {};
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  if (ok)
    *ok = true;
  return ss.str();
}

/** Find `"key"` then skip to `:` and parse bool / number. Keys assumed unique in file. */
size_t findKey(const std::string& j, const std::string& key)
{
  const std::string pat = "\"" + key + "\"";
  return j.find(pat);
}

size_t skipWs(const std::string& j, size_t i)
{
  while (i < j.size() && std::isspace(static_cast<unsigned char>(j[i])))
    ++i;
  return i;
}

size_t afterColon(const std::string& j, size_t key_pos, size_t key_len)
{
  size_t i = skipWs(j, key_pos + key_len);
  if (i >= j.size() || j[i] != ':')
    return std::string::npos;
  return skipWs(j, i + 1);
}

bool parseBoolAt(const std::string& j, size_t i, bool* out)
{
  if (i == std::string::npos || i >= j.size())
    return false;
  if (j.compare(i, 4, "true") == 0) {
    *out = true;
    return true;
  }
  if (j.compare(i, 5, "false") == 0) {
    *out = false;
    return true;
  }
  return false;
}

bool parseNumberAt(const std::string& j, size_t i, double* out)
{
  if (i == std::string::npos || i >= j.size())
    return false;
  try {
    size_t consumed = 0;
    *out = std::stod(j.substr(i), &consumed);
    return consumed > 0;
  } catch (...) {
    return false;
  }
}

void setBool(const std::string& j, const std::string& key, bool& field)
{
  const size_t p = findKey(j, key);
  if (p == std::string::npos)
    return;
  bool v = field;
  if (parseBoolAt(j, afterColon(j, p, key.size() + 2), &v))
    field = v;
}

void setDouble(const std::string& j, const std::string& key, double& field)
{
  const size_t p = findKey(j, key);
  if (p == std::string::npos)
    return;
  double v = field;
  if (parseNumberAt(j, afterColon(j, p, key.size() + 2), &v))
    field = v;
}

}  // namespace

AdasRuntimeConfig loadAdasRuntimeConfig(const std::string& path, bool* ok)
{
  AdasRuntimeConfig cfg;
  bool file_ok = false;
  const std::string j = readFile(path, &file_ok);
  if (!file_ok || j.empty()) {
    LOGE("loadAdasRuntimeConfig: cannot read %s — using defaults", path.c_str());
    if (ok)
      *ok = false;
    return cfg;
  }

  setBool(j, "panda", cfg.panda);
  setBool(j, "zmq_bridge", cfg.zmq_bridge);
  setBool(j, "lane_keep", cfg.lane_keep);
  setBool(j, "localization", cfg.localization);
  setBool(j, "camera_calib", cfg.camera_calib);

  const bool had_imu_key = findKey(j, "imu_calib") != std::string::npos;
  setBool(j, "imu_calib", cfg.imu_calib);
  if (!had_imu_key) {
    // Match previous JNI: IMU calib follows localization when key absent.
    cfg.imu_calib = cfg.localization;
  }

  setDouble(j, "wheelbase_m", cfg.wheelbase_m);
  setDouble(j, "steer_ratio", cfg.steer_ratio);
  setDouble(j, "roll", cfg.roll0_deg);
  setDouble(j, "pitch", cfg.pitch0_deg);
  setDouble(j, "yaw", cfg.yaw0_deg);
  setDouble(j, "z_up", cfg.camera_height_m);
  setDouble(j, "fx", cfg.fx);
  setDouble(j, "fy", cfg.fy);
  setDouble(j, "cx", cfg.cx);
  setDouble(j, "cy", cfg.cy);

  LOGI("loadAdasRuntimeConfig %s: lane_keep=%d loc=%d cam=%d imu=%d wb=%.3f "
       "R/P/Y=%.1f/%.1f/%.1f h=%.2f",
       path.c_str(), cfg.lane_keep ? 1 : 0, cfg.localization ? 1 : 0, cfg.camera_calib ? 1 : 0, cfg.imu_calib ? 1 : 0,
       cfg.wheelbase_m, cfg.roll0_deg, cfg.pitch0_deg, cfg.yaw0_deg, cfg.camera_height_m);

  if (ok)
    *ok = true;
  return cfg;
}

}  // namespace adas
