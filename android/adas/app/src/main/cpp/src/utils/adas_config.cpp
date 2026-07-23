#include "utils/adas_config.h"

#include <fstream>
#include <string>

#include <json/json.h>

#include "utils/logger.h"

namespace {

void setBool(const Json::Value& o, const char* key, bool& field)
{
  if (o.isObject() && o.isMember(key) && o[key].isBool())
    field = o[key].asBool();
}

void setDouble(const Json::Value& o, const char* key, double& field)
{
  if (o.isObject() && o.isMember(key) && o[key].isNumeric())
    field = o[key].asDouble();
}

void setString(const Json::Value& o, const char* key, std::string& field)
{
  if (o.isObject() && o.isMember(key) && o[key].isString()) {
    const std::string v = o[key].asString();
    if (!v.empty())
      field = v;
  }
}

bool parseFile(const std::string& path, Json::Value* root, std::string* err)
{
  std::ifstream in(path);
  if (!in) {
    if (err)
      *err = "cannot open file";
    return false;
  }
  Json::CharReaderBuilder builder;
  builder["collectComments"] = false;
  return Json::parseFromStream(builder, in, root, err);
}

}  // namespace

AdasApp::Config AdasApp::Config::forSimulated(double wheelbase_m, double pitch_deg, double yaw_deg,
                                              double camera_height_m)
{
  Config cfg;
  cfg.feature_flags.enable_panda = false;
  cfg.feature_flags.enable_zmq_bridge = false;
  cfg.feature_flags.enable_lane_keep = true;
  cfg.feature_flags.enable_localization = true;
  cfg.feature_flags.enable_camera_calib = true;
  cfg.feature_flags.enable_imu_calib = true;
  cfg.feature_flags.enable_vision_supercombo = false;

  cfg.lane_keep.wheelbase_m = wheelbase_m;
  cfg.localization.wheelbase_m = wheelbase_m;
  cfg.lane_keep.steer_output_enabled = true;

  cfg.camera_calib.pitch_deg = pitch_deg;
  cfg.camera_calib.yaw_deg = yaw_deg;
  cfg.camera_calib.height_m = camera_height_m;

  cfg.imu_calib.mount_roll_deg = 0.0;
  cfg.imu_calib.mount_pitch_deg = pitch_deg;
  cfg.imu_calib.mount_yaw_deg = yaw_deg;
  cfg.imu_calib.has_mount_prior = true;
  return cfg;
}

AdasApp::Config AdasApp::Config::loadFromFile(const std::string& path, bool* ok)
{
  Config cfg;
  Json::Value root;
  std::string err;
  if (!parseFile(path, &root, &err) || !root.isObject()) {
    LOGE("AdasApp::Config::loadFromFile: cannot read/parse %s (%s) — using defaults", path.c_str(), err.c_str());
    if (ok)
      *ok = false;
    return cfg;
  }

  auto& f = cfg.feature_flags;
  const Json::Value& nodes = root["nodes"];
  setBool(nodes, "panda", f.enable_panda);
  setBool(nodes, "zmq_bridge", f.enable_zmq_bridge);
  setBool(nodes, "lane_keep", f.enable_lane_keep);
  setBool(nodes, "localization", f.enable_localization);
  setBool(nodes, "camera_calib", f.enable_camera_calib);
  setBool(nodes, "vision_supercombo", f.enable_vision_supercombo);

  const bool had_imu_key = nodes.isObject() && nodes.isMember("imu_calib");
  setBool(nodes, "imu_calib", f.enable_imu_calib);
  if (!had_imu_key)
    f.enable_imu_calib = f.enable_localization;

  const Json::Value& veh = root["vehicle"];
  setString(veh, "name", cfg.vehicle_name);
  setDouble(veh, "wheelbase_m", cfg.lane_keep.wheelbase_m);
  setDouble(veh, "wheelbase_m", cfg.localization.wheelbase_m);
  setDouble(veh, "steer_ratio", cfg.lane_keep.steer_ratio);
  setDouble(veh, "steer_ratio", cfg.topic_convert.steer_ratio);
  setDouble(veh, "max_steer_deg", cfg.lane_keep.max_steer_deg);
  setDouble(veh, "max_torque_cnm", cfg.lane_keep.max_torque_cnm);
  setDouble(veh, "pp_k_dd", cfg.lane_keep.pp_k_dd);
  setDouble(veh, "pp_ld_min", cfg.lane_keep.pp_ld_min);
  setDouble(veh, "pp_ld_max", cfg.lane_keep.pp_ld_max);
  setDouble(veh, "pp_shift", cfg.lane_keep.pp_shift);
  setDouble(veh, "lat_pid_kp", cfg.lane_keep.pid_kp);
  setDouble(veh, "lat_pid_ki", cfg.lane_keep.pid_ki);
  setDouble(veh, "lat_pid_kf", cfg.lane_keep.pid_kf);
  setDouble(veh, "steer_sign", cfg.lane_keep.steer_sign);
  cfg.lane_keep.steer_output_enabled = f.enable_lane_keep;

  const Json::Value& cam = root["calibration"]["camera"];
  const Json::Value& rpy = cam["rpy_deg"];
  setDouble(rpy, "roll", cfg.imu_calib.mount_roll_deg);
  setDouble(rpy, "pitch", cfg.camera_calib.pitch_deg);
  setDouble(rpy, "pitch", cfg.imu_calib.mount_pitch_deg);
  setDouble(rpy, "yaw", cfg.camera_calib.yaw_deg);
  setDouble(rpy, "yaw", cfg.imu_calib.mount_yaw_deg);
  cfg.imu_calib.has_mount_prior = rpy.isObject();

  const Json::Value& pos = cam["position_m"];
  setDouble(pos, "z_up", cfg.camera_calib.height_m);

  const Json::Value& K = cam["intrinsics_prior"];
  setDouble(K, "fx", cfg.camera_calib.fx);
  setDouble(K, "fy", cfg.camera_calib.fy);
  setDouble(K, "cx", cfg.camera_calib.cx);
  setDouble(K, "cy", cfg.camera_calib.cy);

  const Json::Value& zmq = root["zmq"];
  setString(zmq, "endpoint_in", cfg.zmq_bridge.endpoint_in);
  setString(zmq, "endpoint_out", cfg.zmq_bridge.endpoint_out);

  LOGI("AdasApp::Config %s: lane_keep=%d loc=%d cam=%d imu=%d wb=%.3f "
       "max_steer=%.1f° max_tq=%.0f pid=%.2f/%.2f/%.5f P/Y=%.1f/%.1f h=%.2f",
       path.c_str(), f.enable_lane_keep ? 1 : 0, f.enable_localization ? 1 : 0, f.enable_camera_calib ? 1 : 0,
       f.enable_imu_calib ? 1 : 0, cfg.lane_keep.wheelbase_m, cfg.lane_keep.max_steer_deg, cfg.lane_keep.max_torque_cnm,
       cfg.lane_keep.pid_kp, cfg.lane_keep.pid_ki, cfg.lane_keep.pid_kf, cfg.camera_calib.pitch_deg,
       cfg.camera_calib.yaw_deg, cfg.camera_calib.height_m);

  if (ok)
    *ok = true;
  return cfg;
}
