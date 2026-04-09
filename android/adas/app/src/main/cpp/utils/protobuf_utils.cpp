#include "protobuf_utils.h"
#include "utils/logger.h"
#include "panda/can.h"
#include "panda/health.h"
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "PandaService", __VA_ARGS__)

namespace utils {
int64_t getCurrentTimestamp()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
      .count();
}

// Panda-specific message creation functions
ai::flow::android::ZMQMessage createCANMessage(const std::vector<can_frame>& frames)
{
  ai::flow::android::ZMQMessage zmq_msg;
  zmq_msg.set_topic("can/rx");

  auto current_timestamp = getCurrentTimestamp();
  zmq_msg.set_timestamp(current_timestamp);

  auto* can_data = zmq_msg.mutable_can_data();
  can_data->set_timestamp(current_timestamp);

  for (const auto& frame : frames) {
    auto* can_frame = can_data->add_frames();
    can_frame->set_address(frame.address);
    can_frame->set_data(frame.dat);
    can_frame->set_bus_time(frame.busTime);
    can_frame->set_src(frame.src);
    LOGI("CAN Frame: addr=0x%03lX, data_size=%zu, busTime=%ld, src=%ld", 
         frame.address, frame.dat.size(), frame.busTime, frame.src);
  }

  return zmq_msg;
}

ai::flow::android::ZMQMessage createHealthMessage(const health_t& health)
{
  ai::flow::android::ZMQMessage zmq_msg;
  zmq_msg.set_topic("panda/health");

  auto current_timestamp = getCurrentTimestamp();
  zmq_msg.set_timestamp(current_timestamp);

  auto* health_data = zmq_msg.mutable_panda_health();
  health_data->set_timestamp(current_timestamp);
  
  // Uptime
  health_data->set_uptime_pkt(health.uptime_pkt);
  
  // Safety status
  health_data->set_controls_allowed(health.controls_allowed_pkt);
  health_data->set_safety_mode(health.safety_mode_pkt);
  health_data->set_safety_param(health.safety_param_pkt);
  health_data->set_fault_status(health.fault_status_pkt);
  
  // Power status
  health_data->set_voltage_mv(health.voltage_pkt);
  health_data->set_current_ma(health.current_pkt);
  health_data->set_power_save_enabled(health.power_save_enabled_pkt);
  
  // TX/RX counters
  health_data->set_tx_blocked(health.safety_tx_blocked_pkt);
  health_data->set_tx_overflow(health.tx_buffer_overflow_pkt);
  health_data->set_rx_invalid(health.safety_rx_invalid_pkt);
  health_data->set_rx_overflow(health.rx_buffer_overflow_pkt);
  health_data->set_rx_checks_invalid(health.safety_rx_checks_invalid_pkt);
  
  // Faults and errors
  health_data->set_faults_pkt(health.faults_pkt);
  health_data->set_spi_error_count(health.spi_error_count_pkt);
  
  // Ignition status
  health_data->set_ignition_line(health.ignition_line_pkt);
  health_data->set_ignition_can(health.ignition_can_pkt);
  
  // Car harness status
  health_data->set_car_harness_status(health.car_harness_status_pkt);
  
  // Heartbeat status
  health_data->set_heartbeat_lost(health.heartbeat_lost_pkt);
  
  // Alternative experience
  health_data->set_alternative_experience(health.alternative_experience_pkt);
  
  // System load
  health_data->set_interrupt_load(health.interrupt_load_pkt);
  
  // Fan status
  health_data->set_fan_power(health.fan_power);
  
  // SBU voltages
  health_data->set_sbu1_voltage_mv(health.sbu1_voltage_mV);
  health_data->set_sbu2_voltage_mv(health.sbu2_voltage_mV);
  
  // System status
  health_data->set_som_reset_triggered(health.som_reset_triggered);

  return zmq_msg;
}
} // namespace utils