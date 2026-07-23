#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <variant>

#include "adas_app.h"

namespace py = pybind11;

PYBIND11_MODULE(core, m)
{
  m.doc() = "ADAS host API: AdasApp (publish → step → pop_messages)";

  py::class_<adas::Vec2>(m, "Vec2")
      .def(py::init<>())
      .def(py::init<double, double>(), py::arg("x"), py::arg("y"))
      .def_property(
          "x", [](const adas::Vec2& v) { return v.x(); }, [](adas::Vec2& v, double val) { v.x() = val; })
      .def_property(
          "y", [](const adas::Vec2& v) { return v.y(); }, [](adas::Vec2& v, double val) { v.y() = val; });

  py::class_<adas::ChassisSample>(m, "ChassisSample")
      .def(py::init<>())
      .def_readwrite("timestamp_us", &adas::ChassisSample::timestamp_us)
      .def_readwrite("speed_mps", &adas::ChassisSample::speed_mps)
      .def_readwrite("steer_rad", &adas::ChassisSample::steer_rad)
      .def_readwrite("yaw_rate", &adas::ChassisSample::yaw_rate);

  py::class_<adas::LanePathMsg>(m, "LanePathMsg")
      .def(py::init<>())
      .def_readwrite("timestamp_us", &adas::LanePathMsg::timestamp_us)
      .def_readwrite("frame_id", &adas::LanePathMsg::frame_id)
      .def_readwrite("polyline", &adas::LanePathMsg::polyline);

  py::class_<adas::GpsSample>(m, "GpsSample")
      .def(py::init<>())
      .def_readwrite("timestamp_us", &adas::GpsSample::timestamp_us)
      .def_readwrite("x", &adas::GpsSample::x)
      .def_readwrite("y", &adas::GpsSample::y)
      .def_readwrite("speed_mps", &adas::GpsSample::speed_mps)
      .def_readwrite("bearing_deg", &adas::GpsSample::bearing_deg)
      .def_readwrite("yaw_enu", &adas::GpsSample::yaw_enu)
      .def_readwrite("vx", &adas::GpsSample::vx)
      .def_readwrite("vy", &adas::GpsSample::vy)
      .def_readwrite("course_valid", &adas::GpsSample::course_valid)
      .def_readwrite("valid", &adas::GpsSample::valid);

  py::class_<adas::ImuSample>(m, "ImuSample")
      .def(py::init<>())
      .def_readwrite("timestamp_us", &adas::ImuSample::timestamp_us)
      .def_readwrite("yaw_rate", &adas::ImuSample::yaw_rate)
      .def_readwrite("valid", &adas::ImuSample::valid);

  py::class_<adas::LocalizationPose>(m, "LocalizationPose")
      .def(py::init<>())
      .def_readonly("timestamp_us", &adas::LocalizationPose::timestamp_us)
      .def_readonly("x", &adas::LocalizationPose::x)
      .def_readonly("y", &adas::LocalizationPose::y)
      .def_readonly("yaw", &adas::LocalizationPose::yaw)
      .def_readonly("v", &adas::LocalizationPose::v)
      .def_readonly("yaw_rate", &adas::LocalizationPose::yaw_rate)
      .def_readonly("odom_x", &adas::LocalizationPose::odom_x)
      .def_readonly("odom_y", &adas::LocalizationPose::odom_y)
      .def_readonly("ekf_x", &adas::LocalizationPose::ekf_x)
      .def_readonly("ekf_y", &adas::LocalizationPose::ekf_y);

  py::class_<adas::LaneKeepOutput>(m, "LaneKeepOutput")
      .def_readonly("timestamp_us", &adas::LaneKeepOutput::timestamp_us)
      .def_readonly("capture_ts_us", &adas::LaneKeepOutput::capture_ts_us)
      .def_readonly("vision_ts_us", &adas::LaneKeepOutput::vision_ts_us)
      .def_readonly("chassis_ts_us", &adas::LaneKeepOutput::chassis_ts_us)
      .def_readonly("publish_ts_us", &adas::LaneKeepOutput::publish_ts_us)
      .def_readonly("steer_rad", &adas::LaneKeepOutput::steer_rad)
      .def_readonly("steer_norm", &adas::LaneKeepOutput::steer_norm)
      .def_readonly("desired_swa_deg", &adas::LaneKeepOutput::desired_swa_deg)
      .def_readonly("actual_swa_deg", &adas::LaneKeepOutput::actual_swa_deg)
      .def_readonly("angle_error_deg", &adas::LaneKeepOutput::angle_error_deg)
      .def_readonly("lookahead_m", &adas::LaneKeepOutput::lookahead_m)
      .def_readonly("target_x", &adas::LaneKeepOutput::target_x)
      .def_readonly("target_y", &adas::LaneKeepOutput::target_y)
      .def_readonly("has_target", &adas::LaneKeepOutput::has_target)
      .def_readonly("curvature", &adas::LaneKeepOutput::curvature)
      .def_readonly("status", &adas::LaneKeepOutput::status);

  py::class_<adas::CameraCalibrationState>(m, "CameraCalibrationState")
      .def_readonly("timestamp_us", &adas::CameraCalibrationState::timestamp_us)
      .def_readonly("roll_deg", &adas::CameraCalibrationState::roll_deg)
      .def_readonly("pitch_deg", &adas::CameraCalibrationState::pitch_deg)
      .def_readonly("yaw_deg", &adas::CameraCalibrationState::yaw_deg)
      .def_readonly("camera_height_m", &adas::CameraCalibrationState::camera_height_m)
      .def_readonly("calibration_success", &adas::CameraCalibrationState::calibration_success)
      .def_readonly("n_updates", &adas::CameraCalibrationState::n_updates)
      .def_readonly("vp_u", &adas::CameraCalibrationState::vp_u)
      .def_readonly("vp_v", &adas::CameraCalibrationState::vp_v)
      .def_readonly("has_vp", &adas::CameraCalibrationState::has_vp)
      .def_readonly("cal_percent", &adas::CameraCalibrationState::cal_percent);

  // Host API: publish inputs → step → read outputs. Algorithms stay inside AdasApp.
  py::class_<AdasApp, std::shared_ptr<AdasApp>>(m, "AdasApp")
      .def(py::init([](double wheelbase, double pitch0_deg, double yaw0_deg, double camera_height,
                       int camera_calib_history_len, double gps_noise_pos, double gps_update_interval) {
             auto app =
                 std::make_shared<AdasApp>(AdasApp::Mode::Simulated, wheelbase, pitch0_deg, yaw0_deg, camera_height,
                                           camera_calib_history_len, gps_noise_pos, gps_update_interval);
             app->start();
             return app;
           }),
           py::arg("wheelbase") = 2.636, py::arg("pitch0_deg") = 0.0, py::arg("yaw0_deg") = 0.0,
           py::arg("camera_height") = 1.22, py::arg("camera_calib_history_len") = 50, py::arg("gps_noise_pos") = 0.5,
           py::arg("gps_update_interval") = 0.2)
      .def("start", &AdasApp::start)
      .def("stop", &AdasApp::stop)
      .def("step", &AdasApp::step, py::arg("timestamp_us"))
      .def("reset_localization", &AdasApp::resetLocalization, py::arg("x") = 0.0, py::arg("y") = 0.0,
           py::arg("yaw") = 0.0, py::arg("v") = 0.0, py::arg("yaw_rate") = 0.0)
      .def("set_camera_intrinsics", &AdasApp::setCameraIntrinsics, py::arg("fx"), py::arg("fy"), py::arg("cx"),
           py::arg("cy"))
      .def("set_camera_estimate", &AdasApp::setCameraEstimate, py::arg("pitch_deg"), py::arg("yaw_deg"))
      .def("set_camera_height", &AdasApp::setCameraHeight, py::arg("height_m"))
      .def("reset_camera_calib", &AdasApp::resetCameraCalib)
      .def("set_lane_keep_pp", &AdasApp::setLaneKeepPp, py::arg("k_dd"), py::arg("ld_min"), py::arg("ld_max"),
           py::arg("shift"))
      .def("set_lane_keep_max_steer_deg", &AdasApp::setLaneKeepMaxSteerDeg, py::arg("max_steer_deg"))
      .def(
          "publish_chassis",
          [](AdasApp& self, int64_t timestamp_us, double speed_mps, double steer_rad, double yaw_rate) {
            adas::ChassisSample c;
            c.timestamp_us = timestamp_us;
            c.speed_mps = speed_mps;
            c.steer_rad = steer_rad;
            c.yaw_rate = yaw_rate;
            self.publishChassis(c);
          },
          py::arg("timestamp_us"), py::arg("speed_mps"), py::arg("steer_rad") = 0.0, py::arg("yaw_rate") = 0.0)
      .def(
          "publish_lanes",
          [](AdasApp& self, int64_t timestamp_us, const std::vector<std::pair<double, double>>& poly, int frame_id) {
            adas::LanePathMsg msg;
            msg.timestamp_us = timestamp_us;
            msg.frame_id = frame_id;
            msg.polyline.reserve(poly.size());
            for (const auto& p : poly)
              msg.polyline.push_back({p.first, p.second});
            self.publishLanes(msg);
          },
          py::arg("timestamp_us"), py::arg("polyline_ego"), py::arg("frame_id") = 0)
      .def(
          "publish_lane_uv",
          [](AdasApp& self, int64_t timestamp_us, const std::vector<std::pair<double, double>>& left,
             const std::vector<std::pair<double, double>>& right) {
            adas::LaneUvMsg msg;
            msg.timestamp_us = timestamp_us;
            msg.left_uv.reserve(left.size());
            msg.right_uv.reserve(right.size());
            for (const auto& p : left)
              msg.left_uv.push_back({p.first, p.second});
            for (const auto& p : right)
              msg.right_uv.push_back({p.first, p.second});
            self.publishLaneUv(msg);
          },
          py::arg("timestamp_us"), py::arg("left_uv"), py::arg("right_uv"))
      .def(
          "publish_gps",
          [](AdasApp& self, int64_t timestamp_us, double x, double y, double speed_mps, double bearing_deg,
             double yaw_enu, double vx, double vy, bool course_valid, bool valid) {
            adas::GpsSample g;
            g.timestamp_us = timestamp_us;
            g.x = x;
            g.y = y;
            g.speed_mps = speed_mps;
            g.bearing_deg = bearing_deg;
            g.yaw_enu = yaw_enu;
            g.vx = vx;
            g.vy = vy;
            g.course_valid = course_valid;
            g.valid = valid;
            self.publishGps(g);
          },
          py::arg("timestamp_us"), py::arg("x"), py::arg("y"), py::arg("speed_mps") = 0.0, py::arg("bearing_deg") = 0.0,
          py::arg("yaw_enu") = 0.0, py::arg("vx") = 0.0, py::arg("vy") = 0.0, py::arg("course_valid") = false,
          py::arg("valid") = true)
      .def(
          "publish_imu",
          [](AdasApp& self, int64_t timestamp_us, double yaw_rate, bool valid) {
            adas::ImuSample imu;
            imu.timestamp_us = timestamp_us;
            imu.yaw_rate = yaw_rate;
            imu.valid = valid;
            self.publishImu(imu);
          },
          py::arg("timestamp_us"), py::arg("yaw_rate"), py::arg("valid") = true)
      .def("pop_messages", [](AdasApp& self) {
        py::list out;
        for (auto& msg : self.popMessages()) {
          std::visit([&](auto&& v) { out.append(py::cast(std::move(v))); }, msg);
        }
        return out;
      });

  m.attr("PyAdasApp") = m.attr("AdasApp");
}
