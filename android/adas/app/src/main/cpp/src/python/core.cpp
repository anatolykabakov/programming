#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <memory>

#include "adas_app.h"
#include "services/camera_calib_service.h"
#include "services/imu_calib_service.h"
#include "services/lane_keep_service.h"
#include "services/localization_service.h"
#include "utils/adas_topics.h"
#include "utils/imu_calibrator.h"
#include "utils/math_utils.h"
#include "utils/online_localizer.h"
#include "utils/pure_pursuit.h"
#include "utils/vehicle_ekf.h"
#include "utils/vanishing_point_calib.h"

namespace py = pybind11;

namespace adas {

/** Python-facing AdasApp in Simulated mode (like navigation::PyNavigationPipeline). */
class PyAdasApp : public AdasApp {
public:
  PyAdasApp(double wheelbase = 2.636, double desired_speed = 12.0, double pitch0_deg = -6.0, double yaw0_deg = 0.0,
            double camera_height = 1.40)
    : AdasApp(AdasApp::Mode::Simulated, wheelbase, desired_speed, pitch0_deg, yaw0_deg, camera_height)
  {
  }

  py::dict popMessages()
  {
    py::dict messages;
    auto& sub = subscriber();
    if (sub.has_new_lane_keep) {
      messages["lane_keep"] = py::cast(sub.lane_keep_);
      sub.has_new_lane_keep = false;
    } else {
      messages["lane_keep"] = py::none();
    }
    if (sub.has_new_pose) {
      messages["localization_pose"] = py::cast(sub.pose_);
      sub.has_new_pose = false;
    } else {
      messages["localization_pose"] = py::none();
    }
    if (sub.has_new_camera_calib) {
      messages["camera_calib"] = py::cast(sub.camera_calib_);
      sub.has_new_camera_calib = false;
    } else {
      messages["camera_calib"] = py::none();
    }
    return messages;
  }
};

// Back-compat alias name used in earlier docs / scripts
using PyAdasPipeline = PyAdasApp;

}  // namespace adas

PYBIND11_MODULE(core, m)
{
  m.doc() = "ADAS algorithms + PyAdasApp (Simulated AdasApp for sim/bag)";

  py::class_<adas::Vec2>(m, "Vec2")
      .def(py::init<>())
      .def(py::init<double, double>(), py::arg("x"), py::arg("y"))
      .def_readwrite("x", &adas::Vec2::x)
      .def_readwrite("y", &adas::Vec2::y);

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
      .def_readwrite("valid", &adas::GpsSample::valid);

  py::class_<adas::ImuSample>(m, "ImuSample")
      .def(py::init<>())
      .def_readwrite("timestamp_us", &adas::ImuSample::timestamp_us)
      .def_readwrite("yaw_rate", &adas::ImuSample::yaw_rate)
      .def_readwrite("valid", &adas::ImuSample::valid);

  py::class_<adas::LocalizationPose>(m, "LocalizationPose")
      .def(py::init<>())
      .def_readwrite("timestamp_us", &adas::LocalizationPose::timestamp_us)
      .def_readwrite("x", &adas::LocalizationPose::x)
      .def_readwrite("y", &adas::LocalizationPose::y)
      .def_readwrite("yaw", &adas::LocalizationPose::yaw)
      .def_readwrite("v", &adas::LocalizationPose::v)
      .def_readwrite("yaw_rate", &adas::LocalizationPose::yaw_rate)
      .def_readwrite("odom_x", &adas::LocalizationPose::odom_x)
      .def_readwrite("odom_y", &adas::LocalizationPose::odom_y)
      .def_readwrite("ekf_x", &adas::LocalizationPose::ekf_x)
      .def_readwrite("ekf_y", &adas::LocalizationPose::ekf_y);

  py::class_<adas::LaneUvMsg>(m, "LaneUvMsg")
      .def(py::init<>())
      .def_readwrite("timestamp_us", &adas::LaneUvMsg::timestamp_us)
      .def_readwrite("left_uv", &adas::LaneUvMsg::left_uv)
      .def_readwrite("right_uv", &adas::LaneUvMsg::right_uv);

  py::class_<adas::CameraCalibrationState>(m, "CameraCalibrationState")
      .def(py::init<>())
      .def_readwrite("timestamp_us", &adas::CameraCalibrationState::timestamp_us)
      .def_readwrite("roll_deg", &adas::CameraCalibrationState::roll_deg)
      .def_readwrite("pitch_deg", &adas::CameraCalibrationState::pitch_deg)
      .def_readwrite("yaw_deg", &adas::CameraCalibrationState::yaw_deg)
      .def_readwrite("camera_height_m", &adas::CameraCalibrationState::camera_height_m)
      .def_readwrite("fx", &adas::CameraCalibrationState::fx)
      .def_readwrite("fy", &adas::CameraCalibrationState::fy)
      .def_readwrite("cx", &adas::CameraCalibrationState::cx)
      .def_readwrite("cy", &adas::CameraCalibrationState::cy)
      .def_readwrite("calibration_success", &adas::CameraCalibrationState::calibration_success)
      .def_readwrite("n_updates", &adas::CameraCalibrationState::n_updates)
      .def_readwrite("vp_u", &adas::CameraCalibrationState::vp_u)
      .def_readwrite("vp_v", &adas::CameraCalibrationState::vp_v)
      .def_readwrite("has_vp", &adas::CameraCalibrationState::has_vp)
      .def_readwrite("cal_percent", &adas::CameraCalibrationState::cal_percent)
      .def_readwrite("cal_status", &adas::CameraCalibrationState::cal_status);

  py::class_<adas::PurePursuitResult>(m, "PurePursuitResult")
      .def_readonly("lookahead_m", &adas::PurePursuitResult::lookahead_m)
      .def_readonly("alpha_rad", &adas::PurePursuitResult::alpha_rad)
      .def_readonly("steer_rad", &adas::PurePursuitResult::steer_rad)
      .def_readonly("speed_mps", &adas::PurePursuitResult::speed_mps)
      .def_readonly("wheel_base", &adas::PurePursuitResult::wheel_base)
      .def_property_readonly("curvature", &adas::PurePursuitResult::curvature)
      .def_property_readonly("has_target", [](const adas::PurePursuitResult& r) { return r.target_ego.has_value(); })
      .def_property_readonly("target_x",
                             [](const adas::PurePursuitResult& r) { return r.target_ego ? r.target_ego->x : 0.0; })
      .def_property_readonly("target_y",
                             [](const adas::PurePursuitResult& r) { return r.target_ego ? r.target_ego->y : 0.0; });

  py::class_<adas::PurePursuit>(m, "PurePursuit")
      .def(py::init<double, double, double, double, double>(), py::arg("K_dd") = 0.4, py::arg("wheel_base") = 2.636,
           py::arg("waypoint_shift") = 1.4, py::arg("ld_min") = 3.0, py::arg("ld_max") = 20.0)
      .def(
          "compute",
          [](const adas::PurePursuit& self, const std::vector<std::pair<double, double>>& poly, double speed) {
            std::vector<adas::Vec2> pts;
            pts.reserve(poly.size());
            for (const auto& p : poly)
              pts.push_back({p.first, p.second});
            return self.compute(pts, speed);
          },
          py::arg("polyline_ego"), py::arg("speed_mps"));

  py::class_<adas::VehicleEKF>(m, "VehicleEKF")
      .def(py::init<double, double, double>(), py::arg("wheelbase") = 2.636, py::arg("gps_noise_pos") = 5.0,
           py::arg("imu_noise_yaw_rate") = 0.02)
      .def("reset", &adas::VehicleEKF::reset, py::arg("x") = 0, py::arg("y") = 0, py::arg("yaw") = 0, py::arg("v") = 0,
           py::arg("yaw_rate") = 0, py::arg("pos_unc") = 10.0, py::arg("yaw_unc") = 0.5, py::arg("v_unc") = 2.0,
           py::arg("yaw_rate_unc") = 0.1)
      .def("predict", &adas::VehicleEKF::predict, py::arg("v_measured"), py::arg("steering_angle"), py::arg("dt"))
      .def("update_gps", &adas::VehicleEKF::updateGps, py::arg("gps_x"), py::arg("gps_y"),
           py::arg("max_innovation") = 50.0)
      .def("update_imu", &adas::VehicleEKF::updateImu, py::arg("yaw_rate_imu"))
      .def_property_readonly("x", &adas::VehicleEKF::x)
      .def_property_readonly("y", &adas::VehicleEKF::y)
      .def_property_readonly("yaw", &adas::VehicleEKF::yaw)
      .def_property_readonly("v", &adas::VehicleEKF::v)
      .def_property_readonly("yaw_rate", &adas::VehicleEKF::yawRate)
      .def_readonly("prediction_count", &adas::VehicleEKF::prediction_count)
      .def_readonly("gps_update_count", &adas::VehicleEKF::gps_update_count)
      .def_readonly("imu_update_count", &adas::VehicleEKF::imu_update_count);

  py::class_<adas::RawImuSample>(m, "RawImuSample")
      .def(py::init<>())
      .def_readwrite("timestamp_us", &adas::RawImuSample::timestamp_us)
      .def_readwrite("ax", &adas::RawImuSample::ax)
      .def_readwrite("ay", &adas::RawImuSample::ay)
      .def_readwrite("az", &adas::RawImuSample::az)
      .def_readwrite("gx", &adas::RawImuSample::gx)
      .def_readwrite("gy", &adas::RawImuSample::gy)
      .def_readwrite("gz", &adas::RawImuSample::gz)
      .def_readwrite("valid", &adas::RawImuSample::valid);

  py::class_<adas::ImuCalibrator>(m, "ImuCalibrator")
      .def(py::init<double, int, int, bool>(), py::arg("speed_threshold_mps") = 0.5 / 3.6, py::arg("min_samples") = 50,
           py::arg("max_buffer") = 400, py::arg("invert_yaw_rate") = true)
      .def("reset", &adas::ImuCalibrator::reset)
      .def("set_speed", &adas::ImuCalibrator::setSpeed, py::arg("speed_mps"))
      .def("set_mount_prior", &adas::ImuCalibrator::setMountPrior, py::arg("roll_deg"), py::arg("pitch_deg"),
           py::arg("yaw_deg"))
      .def(
          "push",
          [](adas::ImuCalibrator& self, const adas::RawImuSample& raw) -> py::object {
            auto yr = self.push(raw);
            if (!yr)
              return py::none();
            return py::float_(*yr);
          },
          py::arg("raw"))
      .def_property_readonly("ready", &adas::ImuCalibrator::ready)
      .def_property_readonly("has_prior", &adas::ImuCalibrator::hasPrior)
      .def_property_readonly("orientation_locked", &adas::ImuCalibrator::orientationLocked)
      .def_property_readonly(
          "bias", [](const adas::ImuCalibrator& c) { return py::make_tuple(c.bias()[0], c.bias()[1], c.bias()[2]); })
      .def_property_readonly("orient_samples", &adas::ImuCalibrator::orient_samples)
      .def_property_readonly("bias_samples", &adas::ImuCalibrator::bias_samples);

  py::class_<adas::ImuCalibService, std::shared_ptr<adas::ImuCalibService>>(m, "ImuCalibService")
      .def(py::init<double, int, bool>(), py::arg("speed_threshold_kmh") = 0.5, py::arg("min_samples") = 50,
           py::arg("invert_yaw_rate") = true)
      .def("set_mount_prior", &adas::ImuCalibService::setMountPrior, py::arg("roll_deg"), py::arg("pitch_deg"),
           py::arg("yaw_deg"))
      .def(
          "push",
          [](adas::ImuCalibService& self, const adas::RawImuSample& raw, double speed) -> py::object {
            auto yr = self.push(raw, speed);
            if (!yr)
              return py::none();
            return py::float_(*yr);
          },
          py::arg("raw"), py::arg("speed_mps"))
      .def_property_readonly("ready", [](const adas::ImuCalibService& s) { return s.calibrator().ready(); })
      .def_property_readonly("last", &adas::ImuCalibService::last);

  py::class_<adas::OnlineLocalizer>(m, "OnlineLocalizer")
      .def(py::init<double, double, double, bool>(), py::arg("wheelbase") = 2.636, py::arg("gps_noise_pos") = 0.5,
           py::arg("gps_update_interval") = 0.2, py::arg("imu_every_step") = true)
      .def("reset", &adas::OnlineLocalizer::reset, py::arg("x") = 0, py::arg("y") = 0, py::arg("yaw") = 0,
           py::arg("v") = 0, py::arg("yaw_rate") = 0)
      .def(
          "step",
          [](adas::OnlineLocalizer& self, double dt, double speed, double steer, py::object yaw_rate, py::object gps_xy,
             py::object ref_xy) {
            std::optional<double> yr;
            std::optional<adas::Vec2> gps, ref;
            if (!yaw_rate.is_none())
              yr = yaw_rate.cast<double>();
            if (!gps_xy.is_none()) {
              auto t = gps_xy.cast<std::pair<double, double>>();
              gps = adas::Vec2{t.first, t.second};
            }
            if (!ref_xy.is_none()) {
              auto t = ref_xy.cast<std::pair<double, double>>();
              ref = adas::Vec2{t.first, t.second};
            }
            auto [x, y, yaw] = self.step(dt, speed, steer, yr, gps, ref);
            return py::make_tuple(x, y, yaw);
          },
          py::arg("dt"), py::arg("speed_mps"), py::arg("steer_rad"), py::arg("yaw_rate") = py::none(),
          py::arg("gps_xy") = py::none(), py::arg("ref_xy") = py::none())
      .def_property_readonly("x", &adas::OnlineLocalizer::x)
      .def_property_readonly("y", &adas::OnlineLocalizer::y)
      .def_property_readonly("yaw", &adas::OnlineLocalizer::yaw)
      .def_property_readonly("ekf_x", &adas::OnlineLocalizer::ekfX)
      .def_property_readonly("ekf_y", &adas::OnlineLocalizer::ekfY)
      .def_property_readonly("odom_x", &adas::OnlineLocalizer::odomX)
      .def_property_readonly("odom_y", &adas::OnlineLocalizer::odomY);

  py::class_<adas::LaneKeepOutput>(m, "LaneKeepOutput")
      .def_readonly("timestamp_us", &adas::LaneKeepOutput::timestamp_us)
      .def_readonly("steer_rad", &adas::LaneKeepOutput::steer_rad)
      .def_readonly("steer_norm", &adas::LaneKeepOutput::steer_norm)
      .def_readonly("throttle", &adas::LaneKeepOutput::throttle)
      .def_readonly("brake", &adas::LaneKeepOutput::brake)
      .def_readonly("lookahead_m", &adas::LaneKeepOutput::lookahead_m)
      .def_readonly("target_x", &adas::LaneKeepOutput::target_x)
      .def_readonly("target_y", &adas::LaneKeepOutput::target_y)
      .def_readonly("has_target", &adas::LaneKeepOutput::has_target)
      .def_readonly("curvature", &adas::LaneKeepOutput::curvature)
      .def_readonly("status", &adas::LaneKeepOutput::status);

  py::class_<adas::LaneKeepService, std::shared_ptr<adas::LaneKeepService>>(m, "LaneKeepService")
      .def(py::init<double, double, double, double, double, double, double, double>(), py::arg("wheelbase") = 2.636,
           py::arg("desired_speed") = 12.0, py::arg("max_steer_deg") = 40.0, py::arg("pp_k_dd") = 0.4,
           py::arg("pp_ld_min") = 3.0, py::arg("pp_ld_max") = 20.0, py::arg("pp_shift") = 1.4,
           py::arg("max_torque_cnm") = 300.0)
      .def(
          "step",
          [](adas::LaneKeepService& self, double speed, const std::vector<std::pair<double, double>>& poly) {
            std::vector<adas::Vec2> pts;
            for (const auto& p : poly)
              pts.push_back({p.first, p.second});
            return self.step(speed, pts);
          },
          py::arg("speed_mps"), py::arg("polyline_ego"));

  py::class_<adas::LocalizationService, std::shared_ptr<adas::LocalizationService>>(m, "LocalizationService")
      .def(py::init<double, double, double>(), py::arg("wheelbase") = 2.636, py::arg("gps_noise_pos") = 0.5,
           py::arg("gps_update_interval") = 0.2)
      .def("reset_pose", &adas::LocalizationService::resetPose, py::arg("x"), py::arg("y"), py::arg("yaw"),
           py::arg("v") = 0, py::arg("yaw_rate") = 0)
      .def(
          "step",
          [](adas::LocalizationService& self, double dt, double speed, double steer, py::object yaw_rate,
             py::object gps_x, py::object gps_y, py::object ref_x, py::object ref_y) {
            std::optional<double> yr, gx, gy, rx, ry;
            if (!yaw_rate.is_none())
              yr = yaw_rate.cast<double>();
            if (!gps_x.is_none())
              gx = gps_x.cast<double>();
            if (!gps_y.is_none())
              gy = gps_y.cast<double>();
            if (!ref_x.is_none())
              rx = ref_x.cast<double>();
            if (!ref_y.is_none())
              ry = ref_y.cast<double>();
            auto [x, y, yaw] = self.step(dt, speed, steer, yr, gx, gy, rx, ry);
            return py::make_tuple(x, y, yaw);
          },
          py::arg("dt"), py::arg("speed_mps"), py::arg("steer_rad"), py::arg("yaw_rate") = py::none(),
          py::arg("gps_x") = py::none(), py::arg("gps_y") = py::none(), py::arg("ref_x") = py::none(),
          py::arg("ref_y") = py::none());

  py::class_<adas::CameraCalibService, std::shared_ptr<adas::CameraCalibService>>(m, "CameraCalibService")
      .def(py::init<double, double, double, double, double, double, double, int>(), py::arg("pitch0_deg") = -6.0,
           py::arg("yaw0_deg") = 0.0, py::arg("height_m") = 1.40, py::arg("fx") = 930.0, py::arg("fy") = 930.0,
           py::arg("cx") = 640.0, py::arg("cy") = 360.0, py::arg("history_len") = 50)
      .def("set_intrinsics", &adas::CameraCalibService::setIntrinsics, py::arg("fx"), py::arg("fy"), py::arg("cx"),
           py::arg("cy"))
      .def("set_height", &adas::CameraCalibService::setHeight, py::arg("height_m"))
      .def("set_estimate", &adas::CameraCalibService::setEstimate, py::arg("pitch_deg"), py::arg("yaw_deg"))
      .def("reset", &adas::CameraCalibService::reset)
      .def(
          "update_from_uv",
          [](adas::CameraCalibService& self, const std::vector<std::pair<double, double>>& left,
             const std::vector<std::pair<double, double>>& right, int64_t t_us) {
            std::vector<adas::Vec2> l, r;
            for (const auto& p : left)
              l.push_back({p.first, p.second});
            for (const auto& p : right)
              r.push_back({p.first, p.second});
            return self.updateFromUv(l, r, t_us);
          },
          py::arg("left_uv"), py::arg("right_uv"), py::arg("timestamp_us") = 0)
      .def(
          "update_from_pose",
          [](adas::CameraCalibService& self, const std::vector<double>& trans, const std::vector<double>& rot,
             const std::vector<double>& trans_std, const std::vector<double>& rot_std, double v_ego, int64_t t_us) {
            adas::CameraOdometrySample o;
            o.timestamp_us = t_us;
            o.valid = trans.size() >= 3 && rot.size() >= 3;
            if (!o.valid)
              return false;
            for (int i = 0; i < 3; ++i) {
              o.trans[i] = trans[i];
              o.rot[i] = rot[i];
              o.trans_std[i] = (trans_std.size() > static_cast<size_t>(i)) ? trans_std[i] : 1.0;
              o.rot_std[i] = (rot_std.size() > static_cast<size_t>(i)) ? rot_std[i] : 1.0;
            }
            return self.updateFromPose(o, v_ego);
          },
          py::arg("trans"), py::arg("rot"), py::arg("trans_std") = std::vector<double>{},
          py::arg("rot_std") = std::vector<double>{}, py::arg("v_ego") = -1.0, py::arg("timestamp_us") = 0)
      .def("set_v_ego", &adas::CameraCalibService::setVEgo, py::arg("v_ego_mps"))
      .def_property_readonly("last", &adas::CameraCalibService::last)
      .def_property_readonly("history_pending", &adas::CameraCalibService::historyPending)
      .def_property_readonly("cal_percent", &adas::CameraCalibService::calPercent);

  py::class_<AdasApp, std::unique_ptr<AdasApp>>(m, "AdasApp")
      .def(py::init(
               [](double wheelbase, double desired_speed, double pitch0_deg, double yaw0_deg, double camera_height) {
                 return std::make_unique<AdasApp>(AdasApp::Mode::Simulated, wheelbase, desired_speed, pitch0_deg,
                                                  yaw0_deg, camera_height);
               }),
           py::arg("wheelbase") = 2.636, py::arg("desired_speed") = 12.0, py::arg("pitch0_deg") = -6.0,
           py::arg("yaw0_deg") = 0.0, py::arg("camera_height") = 1.40)
      .def("publish_chassis", &AdasApp::publishChassis)
      .def("publish_lanes", &AdasApp::publishLanes)
      .def("publish_gps", &AdasApp::publishGps)
      .def("publish_imu", &AdasApp::publishImu)
      .def("publish_lane_uv", &AdasApp::publishLaneUv)
      .def("reset_localization", &AdasApp::resetLocalization, py::arg("x") = 0, py::arg("y") = 0, py::arg("yaw") = 0,
           py::arg("v") = 0, py::arg("yaw_rate") = 0)
      .def("set_camera_intrinsics", &AdasApp::setCameraIntrinsics)
      .def("set_camera_estimate", &AdasApp::setCameraEstimate)
      .def("step", &AdasApp::step, py::arg("timestamp_us"));

  py::class_<adas::PyAdasApp, AdasApp, std::unique_ptr<adas::PyAdasApp>>(m, "PyAdasApp")
      .def(py::init([](double wheelbase, double desired_speed, double pitch0_deg, double yaw0_deg,
                       double camera_height) {
             return std::make_unique<adas::PyAdasApp>(wheelbase, desired_speed, pitch0_deg, yaw0_deg, camera_height);
           }),
           py::arg("wheelbase") = 2.636, py::arg("desired_speed") = 12.0, py::arg("pitch0_deg") = -6.0,
           py::arg("yaw0_deg") = 0.0, py::arg("camera_height") = 1.40)
      .def("pop_messages", &adas::PyAdasApp::popMessages)
      .def(
          "publish_chassis_xy",
          [](adas::PyAdasApp& self, int64_t t_us, double speed, double steer, double yaw_rate) {
            adas::ChassisSample s;
            s.timestamp_us = t_us;
            s.speed_mps = speed;
            s.steer_rad = steer;
            s.yaw_rate = yaw_rate;
            self.publishChassis(s);
          },
          py::arg("timestamp_us"), py::arg("speed_mps"), py::arg("steer_rad"), py::arg("yaw_rate") = 0.0)
      .def(
          "publish_lanes_xy",
          [](adas::PyAdasApp& self, int64_t t_us, const std::vector<std::pair<double, double>>& poly, int frame_id) {
            adas::LanePathMsg msg;
            msg.timestamp_us = t_us;
            msg.frame_id = frame_id;
            for (const auto& p : poly)
              msg.polyline.push_back({p.first, p.second});
            self.publishLanes(msg);
          },
          py::arg("timestamp_us"), py::arg("polyline_ego"), py::arg("frame_id") = 0)
      .def(
          "publish_lane_uv_xy",
          [](adas::PyAdasApp& self, int64_t t_us, const std::vector<std::pair<double, double>>& left,
             const std::vector<std::pair<double, double>>& right) {
            adas::LaneUvMsg msg;
            msg.timestamp_us = t_us;
            for (const auto& p : left)
              msg.left_uv.push_back({p.first, p.second});
            for (const auto& p : right)
              msg.right_uv.push_back({p.first, p.second});
            self.publishLaneUv(msg);
          },
          py::arg("timestamp_us"), py::arg("left_uv"), py::arg("right_uv"));

  // Alias for older name
  m.attr("PyAdasPipeline") = m.attr("PyAdasApp");
  m.attr("AdasPipeline") = m.attr("AdasApp");
}
