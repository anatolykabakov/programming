#include "panda_service.h"
#include "utils/logger.h"
#include "utils/file_logger.h"
#include "utils/protobuf_utils.h"
#include <chrono>
#include <set>

static const std::set<long> ALLOWED_CAN_FRAMES = {
    0xFCU,   // ESC_51 - RX, for wheel speeds
    0x86,    // LWI_01 - steering angle
    0x13DU,  // QFK_01 - RX, for steering angle
    0x300U,  // ACC_01 - RX from ECU, for ACC status
    0x14DU,  // ACC_18 - RX from ECU, for ACC status
    0x12BU,  // GRA_ACC_01 - TX by OP, ACC control buttons for cancel/resume
    0x3BEU,  // MOTOR_14 - RX from ECU, for brake switch status
    0x397U,  // LDW_02 - TX by OP, Lane line recognition and text alerts
    0x10BU,  // Motor_51 - RX TSK state
    0x26BU,  // TA_01 - TX Travel Assist status
    0x1A4U,  // EA_01 - TX EA mitigation
    0x1F0U,  // EA_02 - TX EA mitigation
    0x25DU,  // KLR_01 - TX capacitive steering wheel
    0x303,   // HCA_03 - TX steering
    0x3DC,   // Gateway_73 RX gear
};

const uint16_t SAFETY_NOOUTPUT = 19;     // noOutput @19
const uint16_t SAFETY_MODE_MQBEVO = 29;  // volkswagenMqbEvo @29
const uint16_t SAFETY_PARAM_MQBEVO = 5;
const uint16_t SAFETY_SILENT = 0;  // silent @0
const uint16_t SAFETY_ELM327 = 3;  // elm327 @3

/**
 * @brief Configure the PandaService
 Важные условия СБРОСА controls_allowed:
Событие	Сообщение	Результат
🛑 Кнопка CANCEL	MSG_GRA_ACC_01 bit 13	controls_allowed = FALSE
🔴 ACC выключен	MSG_Motor_51 acc_main_on=0	controls_allowed = FALSE
🚗 Педаль тормоза	(generic_rx_checks)	controls_allowed = FALSE
⛽ Педаль газа	(generic_rx_checks)	controls_allowed = FALSE
💓 Heartbeat lost	(3 sec timeout)	controls_allowed = FALSE
 */

void PandaService::configure()
{
  try {
    initializePanda();

    subscribe<ai::flow::android::ZMQMessage>("can/tx",
                                             std::bind(&PandaService::canTxCallback, this, std::placeholders::_1));
    scheduleTimer(50000, std::bind(&PandaService::pandaRxCallback, this));      // 50ms
    scheduleTimer(100000, std::bind(&PandaService::pandaStateCallback, this));  // 100ms

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
    
    for (int i = 0; i < 3; i++) {
      panda_->set_can_fd_auto(i, true);
    }

  } catch (const std::exception& e) {
    LOGE("Failed to initialize Panda: %s", e.what());
    throw;
  }
}

void PandaService::canTxCallback(const ai::flow::android::ZMQMessage& msg)
{
  if (!panda_ || !panda_->connected() || !panda_->comms_healthy()) {
    LOGD("Panda not ready");
    return;
  }

  std::vector<can_frame> frames;
  if (msg.has_can_data()) {
    const auto& can_tx_frames = msg.can_data().frames();
    for (const auto& frame : can_tx_frames) {
      frames.push_back(
          can_frame{.address = frame.address(), .dat = frame.data(), .busTime = frame.bus_time(), .src = frame.src()});
    }
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
      LOGE("❌ Failed to set safe mode on shutdown");
    }
  }
}

void PandaService::reset() {}

void PandaService::pandaRxCallback()
{
  if (!panda_ || !panda_->connected()) {
    LOGD("Panda not connected");
    return;
  }

  try {
    std::vector<can_frame> raw_can_data;
    if (!panda_->can_receive(raw_can_data)) {
      LOGD("Failed to receive CAN data from Panda");
      return;
    }

    if (raw_can_data.empty()) {
      LOGD("No CAN data received this iteration");
      return;
    }

    LOGI("Processing %zu CAN frames from Panda", raw_can_data.size());

    std::vector<can_frame> filtered_frames;
    for (const auto& frame : raw_can_data) {
      if (ALLOWED_CAN_FRAMES.count(frame.address) > 0) {
        filtered_frames.push_back(frame);
      }
    }

    if (filtered_frames.empty()) {
      LOGD("No relevant CAN frames to send (filtered %zu frames)", raw_can_data.size());
      return;
    }

    publish("can/rx", utils::createCANMessage(filtered_frames));
  } catch (const std::exception& e) {
    LOGE("Exception in pandaTimerCallback(): %s", e.what());
  }
} 

void PandaService::pandaStateCallback()
{
  if (!panda_ || !panda_->connected()) {
    LOGD("Panda not connected for health check");
    return;
  }
  bool ignition_local = false;

  try {
    auto state = panda_->get_state();
    if (state) {
      // Make sure CAN buses are live: safety_setter_thread does not work if Panda CAN are silent and there is only one
      // other CAN node
      if (state->safety_mode_pkt == SAFETY_SILENT) {
        panda_->set_safety_model(SAFETY_NOOUTPUT, 0);
      }

      ignition_local |= ((state->ignition_line_pkt != 0) || (state->ignition_can_pkt != 0));
      bool power_save_desired = !ignition_local;
      if (state->power_save_enabled_pkt != power_save_desired) {
        panda_->set_power_saving(power_save_desired);
      }

      bool is_onroad = false;
      bool should_close_relay = !ignition_local || !is_onroad;
      if (should_close_relay && (state->safety_mode_pkt != SAFETY_NOOUTPUT)) {
        panda_->set_safety_model(SAFETY_NOOUTPUT, 0);
      }

      panda_->send_heartbeat(engaged_, engaged_mads_);

      if (!initialized_) {
        initialized_ = true;
        panda_->set_safety_model(SAFETY_ELM327, 1U);
      }

      if (is_onroad && !safety_configured_) {
        panda_->set_safety_model(SAFETY_MODE_MQBEVO, SAFETY_PARAM_MQBEVO);
        safety_configured_ = true;
      } else if (!is_onroad) {
        safety_configured_ = false;
      }
      // Publish health data to internal topic
      publish("panda/health", utils::createHealthMessage(*state));
    }
  } catch (const std::exception& e) {
    LOGE("Exception in pandaStateCallback(): %s", e.what());
  }
}
