#include "services/panda_service.h"
#include "utils/logger.h"
#include "utils/protobuf_utils.h"
#include "volkswagen/values.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <set>
#include <thread>
#include <tuple>

namespace {

// VW Golf 7 MQB — dragonpilot volkswagen safety @15 / carstate signals.
const std::set<long> ALLOWED_CAN_FRAMES = {
    0x0B2,  // ESP_19 — wheel speeds
    0x086,  // LWI_01 — steering angle
    0x09F,  // LH_EPS_03 — driver steering torque
    0x0AD,  // Getriebe_11 — gear
    0x0FD,  // ESP_21 — vehicle speed (aux)
    0x101,  // ESP_02 — yaw rate
    0x106,  // ESP_05 — brake
    0x120,  // TSK_06 — ACC status
    0x121,  // MOTOR_20 — throttle
    0x122,  // ACC_06
    0x126,  // HCA_01
    0x12B,  // GRA_ACC_01
    0x12E,  // ACC_07
    0x30C,  // ACC_02
    0x397,  // LDW_02
    0x3BE,  // MOTOR_14 — brake pedal
};

constexpr uint16_t SAFETY_VOLKSWAGEN = 15;
constexpr uint16_t SAFETY_NOOUTPUT = 19;
constexpr uint16_t SAFETY_PARAM_STOCK = 0;
// Must be set while NOT in car safety mode (see panda 0xdf). Softens gas→disengage.
constexpr uint16_t ALT_EXP_DISABLE_DISENGAGE_ON_GAS = 1;

constexpr double KPH_TO_MS = 1.0 / 3.6;
constexpr double DEG_TO_RAD = 0.017453292519943295;
constexpr double STEER_DRIVER_ALLOWANCE = 80.0;
constexpr int64_t HCA_CMD_TIMEOUT_MS = 250;

// Voltage hysteresis for "car awake" — brief dips must not call set_safety (resets allowed).
constexpr uint32_t IGN_VOLTAGE_ON_MV = 11500;
constexpr uint32_t IGN_VOLTAGE_OFF_MV = 10500;
constexpr int64_t IGN_OFF_DEBOUNCE_MS = 3000;

double signed_signal(double mag, double sign_bit) { return mag * (sign_bit > 0.5 ? -1.0 : 1.0); }

}  // namespace

PandaService::PandaService(int usb_fd, std::string dbc_path) : usb_fd_(usb_fd), dbc_path_(std::move(dbc_path)) {}

void PandaService::configure()
{
  try {
    if (!dbc_path_.empty()) {
      dbc_ = std::make_unique<DBSParser>(dbc_path_);
      LOGI("Loaded DBC: %s (%zu messages)", dbc_path_.c_str(), dbc_->getAllMessages().size());
    } else {
      LOGW("No DBC path — CarState decode disabled");
    }

    initializePanda();

    subscribe<ai::flow::adas::ZMQMessage>("controls/steer",
                                          std::bind(&PandaService::steerCommandCallback, this, std::placeholders::_1));
    scheduleTimer(50000, std::bind(&PandaService::pandaRxCallback, this));
    scheduleTimer(100000, std::bind(&PandaService::pandaStateCallback, this));
    scheduleTimer(10000, std::bind(&PandaService::carControllerCallback, this));

    LOGI("PandaService configured successfully");
  } catch (const std::exception& e) {
    LOGE("Failed to configure PandaService: %s", e.what());
    throw;
  }
}

void PandaService::initializePanda()
{
  LOGI("Initializing Panda device...");
  try {
    panda_ = std::make_shared<Panda>(usb_fd_, 0);
    LOGI("Panda instance created successfully");
  } catch (const std::exception& e) {
    LOGE("Failed to initialize Panda: %s", e.what());
    throw;
  }
}

void PandaService::steerCommandCallback(const ai::flow::adas::ZMQMessage& msg)
{
  if (!msg.has_steer_command()) {
    return;
  }
  const auto& cmd = msg.steer_command();
  int torque = cmd.torque_cnm();
  if (torque > 300) {
    torque = 300;
  } else if (torque < -300) {
    torque = -300;
  }
  hca_cmd_steer_ = torque;
  hca_cmd_enabled_ = cmd.enabled() && (torque != 0);
  hca_cmd_ts_ms_ = utils::getCurrentTimestamp();
}

volkswagen::CarStateView PandaService::buildCarStateView() const
{
  volkswagen::CarStateView cs;
  cs.vEgo = car_state_.v_ego();
  cs.standstill = car_state_.standstill() || (std::abs(car_state_.v_ego()) < 0.3f);
  cs.steeringTorque = car_state_.steering_torque();
  cs.steeringPressed = car_state_.steering_pressed();
  cs.epsHcaStatus = eps_hca_status_;
  cs.ldwStock = ldw_stock_;
  return cs;
}

void PandaService::carControllerCallback()
{
  if (!panda_ || !panda_->connected() || !panda_->comms_healthy()) {
    return;
  }
  // Stock camera HCA/LDW blocked by volkswagen fwd_hook — only TX while safety live.
  if (last_safety_mode_ != SAFETY_VOLKSWAGEN || !last_ignition_) {
    return;
  }

  const int64_t now = utils::getCurrentTimestamp();
  const bool cmd_fresh = hca_cmd_ts_ms_ > 0 && (now - hca_cmd_ts_ms_) <= HCA_CMD_TIMEOUT_MS;

  volkswagen::CarControl cc;
  // latActive = inject + controls_allowed. EPS/standstill gated only in CarController.
  cc.latActive = cmd_fresh && hca_cmd_enabled_ && last_controls_allowed_;
  if (cmd_fresh) {
    cc.actuators.steerTorqueCNm = hca_cmd_steer_;
  }
  // Lane lines visible → green LDW LED when lat active (stock LDW blocked).
  cc.hud.leftLaneVisible = true;
  cc.hud.rightLaneVisible = true;

  auto frames = car_controller_.update(cc, buildCarStateView());
  if (!frames.empty()) {
    panda_->can_send(frames);
  }
}

PandaService::~PandaService()
{
  if (panda_ && panda_->connected()) {
    try {
      panda_->set_safety_model(SAFETY_NOOUTPUT, 0);
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    } catch (...) {
      LOGE("Failed to set safe mode on shutdown");
    }
  }
}

void PandaService::reset() {}

void PandaService::updateCarStateFromFrame(const can_frame& frame)
{
  if (!dbc_) {
    return;
  }

  switch (frame.address) {
    case 0x0B2: {  // ESP_19
      auto fl = dbc_->extractSignal(frame, "ESP_VL_Radgeschw_02");
      auto fr = dbc_->extractSignal(frame, "ESP_VR_Radgeschw_02");
      auto rl = dbc_->extractSignal(frame, "ESP_HL_Radgeschw_02");
      auto rr = dbc_->extractSignal(frame, "ESP_HR_Radgeschw_02");
      if (fl && fr && rl && rr) {
        const float fl_ms = static_cast<float>(*fl * KPH_TO_MS);
        const float fr_ms = static_cast<float>(*fr * KPH_TO_MS);
        const float rl_ms = static_cast<float>(*rl * KPH_TO_MS);
        const float rr_ms = static_cast<float>(*rr * KPH_TO_MS);
        auto* ws = car_state_.mutable_wheel_speeds();
        ws->set_fl(fl_ms);
        ws->set_fr(fr_ms);
        ws->set_rl(rl_ms);
        ws->set_rr(rr_ms);

        const double v_raw = (fl_ms + fr_ms + rl_ms + rr_ms) * 0.25;
        const int64_t now = utils::getCurrentTimestamp();
        float a_ego = car_state_.a_ego();
        if (prev_v_ts_ms_ > 0 && now > prev_v_ts_ms_) {
          a_ego = static_cast<float>((v_raw - prev_v_ego_) / ((now - prev_v_ts_ms_) * 1e-3));
        }
        prev_v_ego_ = v_raw;
        prev_v_ts_ms_ = now;

        car_state_.set_v_ego_raw(static_cast<float>(v_raw));
        car_state_.set_v_ego(static_cast<float>(v_raw));
        car_state_.set_a_ego(a_ego);
        car_state_.set_standstill(v_raw < 0.05);
        car_state_dirty_ = true;
      }
      break;
    }
    case 0x086: {  // LWI_01
      auto angle = dbc_->extractSignal(frame, "LWI_Lenkradwinkel");
      auto asign = dbc_->extractSignal(frame, "LWI_VZ_Lenkradwinkel");
      auto rate = dbc_->extractSignal(frame, "LWI_Lenkradw_Geschw");
      auto rsign = dbc_->extractSignal(frame, "LWI_VZ_Lenkradw_Geschw");
      if (angle && asign) {
        car_state_.set_steering_angle_deg(static_cast<float>(signed_signal(*angle, *asign)));
        car_state_dirty_ = true;
      }
      if (rate && rsign) {
        car_state_.set_steering_rate_deg(static_cast<float>(signed_signal(*rate, *rsign)));
        car_state_dirty_ = true;
      }
      break;
    }
    case 0x09F: {  // LH_EPS_03
      auto torq = dbc_->extractSignal(frame, "EPS_Lenkmoment");
      auto tsign = dbc_->extractSignal(frame, "EPS_VZ_Lenkmoment");
      if (torq && tsign) {
        const double t = signed_signal(*torq, *tsign);
        car_state_.set_steering_torque(static_cast<float>(t));
        car_state_.set_steering_pressed(std::abs(t) > STEER_DRIVER_ALLOWANCE);
        car_state_dirty_ = true;
      }
      auto hca_st = dbc_->extractSignal(frame, "EPS_HCA_Status");
      if (hca_st) {
        eps_hca_status_ = static_cast<uint8_t>(*hca_st);
      }
      break;
    }
    case 0x101: {  // ESP_02
      auto yaw = dbc_->extractSignal(frame, "ESP_Gierrate");
      auto ysign = dbc_->extractSignal(frame, "ESP_VZ_Gierrate");
      if (yaw && ysign) {
        car_state_.set_yaw_rate(static_cast<float>(signed_signal(*yaw, *ysign) * DEG_TO_RAD));
        car_state_dirty_ = true;
      }
      break;
    }
    case 0x121: {  // Motor_20
      auto gas = dbc_->extractSignal(frame, "MO_Fahrpedalrohwert_01");
      if (gas) {
        const float g = static_cast<float>(*gas / 100.0);
        car_state_.set_gas(g);
        car_state_.set_gas_pressed(g > 0.f);
        car_state_dirty_ = true;
      }
      break;
    }
    case 0x106: {  // ESP_05
      auto pressure = dbc_->extractSignal(frame, "ESP_Bremsdruck");
      auto driver_brake = dbc_->extractSignal(frame, "ESP_Fahrer_bremst");
      if (pressure) {
        car_state_.set_brake(static_cast<float>(*pressure / 250.0));
        car_state_dirty_ = true;
      }
      if (driver_brake) {
        brake_esp_ = *driver_brake > 0.5;
        car_state_.set_brake_pressed(brake_esp_ || brake_motor_);
        car_state_dirty_ = true;
      }
      break;
    }
    case 0x3BE: {  // MOTOR_14
      auto pedal = dbc_->extractSignal(frame, "MO_Fahrer_bremst");
      if (pedal) {
        brake_motor_ = *pedal > 0.5;
        car_state_.set_brake_pressed(brake_esp_ || brake_motor_);
        car_state_dirty_ = true;
      }
      break;
    }
    case 0x0AD: {  // Getriebe_11
      auto gear = dbc_->extractSignal(frame, "GE_Fahrstufe");
      if (gear) {
        car_state_.set_gear(static_cast<int32_t>(*gear));
        car_state_dirty_ = true;
      }
      break;
    }
    case 0x12B: {  // GRA_ACC_01 — stalk decode only (no GRA TX / cancel-resume spoof)
      auto set_btn = [&](const char* name, void (ai::flow::adas::CarState::*setter)(bool)) {
        if (auto v = dbc_->extractSignal(frame, name)) {
          (car_state_.*setter)(*v > 0.5);
          car_state_dirty_ = true;
        }
      };
      set_btn("GRA_Hauptschalter", &ai::flow::adas::CarState::set_cruise_main_switch);
      set_btn("GRA_Tip_Setzen", &ai::flow::adas::CarState::set_cruise_set);
      set_btn("GRA_Tip_Wiederaufnahme", &ai::flow::adas::CarState::set_cruise_resume);
      set_btn("GRA_Abbrechen", &ai::flow::adas::CarState::set_cruise_cancel);
      set_btn("GRA_Tip_Hoch", &ai::flow::adas::CarState::set_cruise_accel);
      set_btn("GRA_Tip_Runter", &ai::flow::adas::CarState::set_cruise_decel);
      if (auto gap = dbc_->extractSignal(frame, "GRA_Verstellung_Zeitluecke")) {
        car_state_.set_cruise_gap_adjust(static_cast<int32_t>(*gap));
        car_state_dirty_ = true;
      }
      break;
    }
    case 0x397: {  // LDW_02 — stock camera HUD (skip our own TX echo src>=128)
      if (frame.dat.size() >= 8 && (frame.src & 0x80) == 0) {
        std::memcpy(ldw_stock_.data, frame.dat.data(), 8);
        ldw_stock_.valid = true;
      }
      break;
    }
    case 0x120: {  // TSK_06 — ACC status (enables HCA / controls_allowed)
      if (auto st = dbc_->extractSignal(frame, "TSK_Status")) {
        const int status = static_cast<int>(*st);
        if (status != last_tsk_status_) {
          LOGW("TSK_Status %d → %d (pcm cruise %s; panda allowed needs rising edge 3/4/5)", last_tsk_status_, status,
               ((status == 3) || (status == 4) || (status == 5)) ? "engaged" : "NOT engaged");
          last_tsk_status_ = status;
        }
        car_state_.set_acc_status(status);
        // Match panda volkswagen_mqb_rx_hook / pcm_cruise_check
        const bool engaged = (status == 3) || (status == 4) || (status == 5);
        const bool available = engaged || (status == 2);
        car_state_.set_cruise_engaged(engaged);
        car_state_.set_cruise_available(available);
        car_state_dirty_ = true;
      }
      break;
    }
    default:
      break;
  }
}

void PandaService::publishCarState()
{
  if (!car_state_dirty_) {
    return;
  }
  car_state_dirty_ = false;
  car_state_.set_timestamp(utils::getCurrentTimestamp());
  publish("vehicle/state", utils::createCarStateMessage(car_state_));
}

void PandaService::pandaRxCallback()
{
  if (!panda_ || !panda_->connected()) {
    return;
  }

  try {
    std::vector<can_frame> raw_can_data;
    if (!panda_->can_receive(raw_can_data) || raw_can_data.empty()) {
      return;
    }

    std::vector<can_frame> filtered_frames;
    for (const auto& frame : raw_can_data) {
      if (ALLOWED_CAN_FRAMES.count(frame.address) == 0) {
        continue;
      }
      filtered_frames.push_back(frame);
      updateCarStateFromFrame(frame);
    }

    if (!filtered_frames.empty()) {
      publish("can/rx", utils::createCANMessage(filtered_frames));
    }
    publishCarState();
  } catch (const std::exception& e) {
    LOGE("Exception in pandaRxCallback(): %s", e.what());
  }
}

void PandaService::pandaStateCallback()
{
  if (!panda_ || !panda_->connected()) {
    return;
  }

  try {
    auto state = panda_->get_state();
    if (!state) {
      return;
    }

    // Panda ignition_line/can often stay 0 on MQB harness; use rail voltage with hysteresis.
    const bool ignition_hw = (state->ignition_line_pkt != 0) || (state->ignition_can_pkt != 0);
    const int64_t now_ms = utils::getCurrentTimestamp();
    const bool voltage_on = state->voltage_pkt >= IGN_VOLTAGE_ON_MV;
    const bool voltage_off = state->voltage_pkt < IGN_VOLTAGE_OFF_MV;
    if (ignition_hw || voltage_on) {
      ignition_sticky_ = true;
      ignition_low_since_ms_ = 0;
    } else if (voltage_off) {
      if (ignition_low_since_ms_ == 0) {
        ignition_low_since_ms_ = now_ms;
      } else if ((now_ms - ignition_low_since_ms_) >= IGN_OFF_DEBOUNCE_MS) {
        ignition_sticky_ = false;
      }
    } else {
      // Between thresholds: keep previous sticky, reset off-timer.
      ignition_low_since_ms_ = 0;
    }
    const bool ignition = ignition_sticky_;

    constexpr uint16_t want = SAFETY_VOLKSWAGEN;
    constexpr uint16_t param = SAFETY_PARAM_STOCK;

    if (!initialized_) {
      initialized_ = true;
      if (auto vers = panda_->get_packets_versions()) {
        LOGI("Panda packet versions health=%u can=%u can_health=%u", std::get<0>(*vers), std::get<1>(*vers),
             std::get<2>(*vers));
      }
      LOGI("Panda init V=%u ign_hw=%d ign=%d safety_now=%u", state->voltage_pkt, ignition_hw ? 1 : 0, ignition ? 1 : 0,
           state->safety_mode_pkt);

      // CRITICAL: set_safety_hooks() resets controls_allowed — call only when mode is wrong.
      // VW safety ignores 0xf8 heartbeat_disabled; send_heartbeat(true) is enough.
      panda_->set_power_saving(false);

      // ALT_EXP only accepted in non-car safety — set while NOOUTPUT then enter VW.
      if (state->safety_mode_pkt != SAFETY_NOOUTPUT) {
        panda_->set_safety_model(SAFETY_NOOUTPUT, 0);
      }
      panda_->set_alternative_experience(ALT_EXP_DISABLE_DISENGAGE_ON_GAS, 0);
      alt_exp_configured_ = true;
      LOGI("Panda alt_exp DISABLE_DISENGAGE_ON_GAS set (before VW safety)");

      if (ignition) {
        panda_->set_safety_model(want, param);
        if (auto after = panda_->get_state()) {
          LOGI("Panda initial safety want=%u got=%u allowed=%d hb_lost=%d psave=%d alt=%u", want,
               after->safety_mode_pkt, after->controls_allowed_pkt ? 1 : 0, after->heartbeat_lost_pkt ? 1 : 0,
               after->power_save_enabled_pkt ? 1 : 0, after->alternative_experience_pkt);
          safety_configured_ = (after->safety_mode_pkt == want);
          state = after;
        }
      }
    }

    // power_save only when drifted
    const bool power_save_desired = !ignition;
    if (state->power_save_enabled_pkt != static_cast<uint8_t>(power_save_desired)) {
      panda_->set_power_saving(power_save_desired);
      LOGW("Panda power_save %d → %d", state->power_save_enabled_pkt ? 1 : 0, power_save_desired ? 1 : 0);
    }

    if (ignition) {
      // Re-set safety ONLY if mode drifted — never every tick (resets controls_allowed).
      if (state->safety_mode_pkt != want) {
        // Re-apply alt_exp if we bounced through NOOUTPUT.
        if (!alt_exp_configured_ || state->safety_mode_pkt == SAFETY_NOOUTPUT ||
            state->safety_mode_pkt == 0 /* SILENT */) {
          if (state->safety_mode_pkt != SAFETY_NOOUTPUT) {
            panda_->set_safety_model(SAFETY_NOOUTPUT, 0);
          }
          panda_->set_alternative_experience(ALT_EXP_DISABLE_DISENGAGE_ON_GAS, 0);
          alt_exp_configured_ = true;
        }
        LOGW("Panda safety drifted to %u — re-set %u (allowed will clear; press ACC Set again)", state->safety_mode_pkt,
             want);
        panda_->set_safety_model(want, param);
        if (auto after = panda_->get_state()) {
          safety_configured_ = (after->safety_mode_pkt == want);
          if (!safety_configured_) {
            LOGW("Panda safety DID NOT STICK (want %u, still %u)", want, after->safety_mode_pkt);
          }
          state = after;
        }
      } else {
        safety_configured_ = true;
      }
    } else {
      // Only after sticky ignition false (debounced) — avoids wiping allowed on V dips.
      if (state->safety_mode_pkt != SAFETY_NOOUTPUT) {
        LOGW("Panda ignition off (debounced) → NOOUTPUT");
        panda_->set_safety_model(SAFETY_NOOUTPUT, 0);
        alt_exp_configured_ = false;  // re-apply before next VW enter
      }
      safety_configured_ = false;
    }

    // While VW safety is live, ALWAYS heartbeat engaged=1.
    // FW: controls_allowed && !heartbeat_engaged for 3×1Hz → clears allowed.
    const bool hb_engaged = (state->safety_mode_pkt == want);
    panda_->send_heartbeat(hb_engaged);

    const bool allowed_now = state->controls_allowed_pkt != 0;
    if (allowed_now != last_controls_allowed_) {
      LOGW("controls_allowed %d → %d | safety=%u TSK=%d cruise_eng=%d brake=%d gas=%d V=%u hb_eng=%d",
           last_controls_allowed_ ? 1 : 0, allowed_now ? 1 : 0, state->safety_mode_pkt, last_tsk_status_,
           car_state_.cruise_engaged() ? 1 : 0, car_state_.brake_pressed() ? 1 : 0, car_state_.gas_pressed() ? 1 : 0,
           state->voltage_pkt, hb_engaged ? 1 : 0);
    }

    last_safety_mode_ = state->safety_mode_pkt;
    last_controls_allowed_ = allowed_now;
    last_ignition_ = ignition;

    publish("panda/health", utils::createHealthMessage(*state));

    static int safety_log_div = 0;
    if (++safety_log_div >= 50) {
      safety_log_div = 0;
      LOGI("Panda health: safety=%u param=%u allowed=%d hb_lost=%d psave=%d V=%u ign_hw=%d ign=%d configured=%d "
           "hb_eng=%d TSK=%d eps_hca=%u(%s) apply=%d cmd=%d lat=%d ldw=%d",
           state->safety_mode_pkt, state->safety_param_pkt, state->controls_allowed_pkt ? 1 : 0,
           state->heartbeat_lost_pkt ? 1 : 0, state->power_save_enabled_pkt ? 1 : 0, state->voltage_pkt,
           ignition_hw ? 1 : 0, ignition ? 1 : 0, safety_configured_ ? 1 : 0, hb_engaged ? 1 : 0, last_tsk_status_,
           eps_hca_status_, volkswagen::epsHcaStatusName(eps_hca_status_), car_controller_.applySteerLast(),
           hca_cmd_steer_, (hca_cmd_enabled_ && last_controls_allowed_) ? 1 : 0, ldw_stock_.valid ? 1 : 0);
    }
  } catch (const std::exception& e) {
    LOGE("Exception in pandaStateCallback(): %s", e.what());
  }
}
