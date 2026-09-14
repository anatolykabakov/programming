// TSR in Clean Architecture — transitive skeleton with simplified real TSR logic.
//
// Based on atom::tsr StateProcessor transitions (Init/Off/Failure/Standby/Active + L1 substates).
//
// Build:
//   g++ -std=c++17 examples/tsr_clean_arch_example.cpp -o tsr_example -pthread
// Run:
//   ./tsr_example

#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

// =============================================================================
// DOMAIN
// =============================================================================

enum class VehicleState { Sleep, Standby, Drive };
enum class Gear { Unspecified, P, N, R, D };

struct CanInput {
  VehicleState vehicle_state{VehicleState::Sleep};
  Gear gear{Gear::Unspecified};
  float speed_kmh{0};
  bool speed_valid{false};
  bool eps_valid{false};
  bool acu_valid{false};
  bool camera_ok{true};
  bool turning{false};
  float yaw_rate{0};
};

struct SpeedLimitSign {
  int limit_kmh{0};
  bool is_end_sign{false};
  bool has_explicit_value{false};
};

struct InputSnapshot {
  bool tsr_feature_on{true};  // config.tsr_on in real TSR
  bool ivi_tsr_on{false};     // IVI ON/OFF command
  CanInput can;
  std::vector<SpeedLimitSign> signs;
};

enum class Level0 { Init, Off, Failure, Standby, Active };
enum class Level1 { None, NoDisplay, Display, ContinueDisplayingPermanent };

struct StateType {
  Level0 l0{Level0::Init};
  Level1 l1{Level1::None};

  void normalize()
  {
    if (l0 == Level0::Active) {
      if (l1 == Level1::None)
        l1 = Level1::NoDisplay;
    } else {
      l1 = Level1::None;
    }
  }

  bool isOn() const { return l0 == Level0::Standby || l0 == Level0::Active; }
};

struct TsrOutput {
  std::string topic;
  std::string payload;
};

struct TsrInternalContext {
  std::optional<int> last_displayed_limit;
  TimePoint yaw_turn_start{};
  TimePoint yaw_u_turn_start{};
};

struct TsrTickResult {
  StateType state;
  std::vector<TsrOutput> messages;
};

class TsrStateMachine {
public:
  static constexpr int kYawTurnThresholdMs = 500;
  static constexpr int kYawUTurnThresholdMs = 1000;
  static constexpr float kYawRateForTurn = 10.f;
  static constexpr float kYawRateForUTurn = 20.f;
  static constexpr float kMaxUTurnSpeed = 20.f;

  TsrTickResult tick(StateType state, TsrInternalContext& ctx, const InputSnapshot& in, TimePoint now) const
  {
    TsrTickResult result{state, {}};
    const StateType old = result.state;

    updateYawTimers(ctx, in.can, now);
    processTopLevel(result.state, in);
    if (result.state.l0 == Level0::Active)
      processActiveSubstate(result.state, in, ctx, now);
    result.state.normalize();

    if (!result.state.isOn())
      ctx = {};

    processSpeedLimits(result, ctx, in);
    pushStateMessages(result, old);

    return result;
  }

  static std::string stateName(const StateType& s)
  {
    std::string name;
    switch (s.l0) {
      case Level0::Init:
        return "Init";
      case Level0::Off:
        return "Off";
      case Level0::Failure:
        return "Failure";
      case Level0::Standby:
        return "Standby";
      case Level0::Active:
        name = "Active";
        break;
    }
    switch (s.l1) {
      case Level1::NoDisplay:
        return name + "/NoDisplay";
      case Level1::Display:
        return name + "/Display";
      case Level1::ContinueDisplayingPermanent:
        return name + "/ContinueDisplayingPermanent";
      default:
        return name;
    }
  }

private:
  static void updateYawTimers(TsrInternalContext& ctx, const CanInput& can, TimePoint now)
  {
    auto update = [&](TimePoint& start, float threshold) {
      if (std::abs(can.yaw_rate) > threshold) {
        if (start == TimePoint{})
          start = now;
      } else {
        start = TimePoint{};
      }
    };
    update(ctx.yaw_turn_start, kYawRateForTurn);
    update(ctx.yaw_u_turn_start, kYawRateForUTurn);
  }

  static bool isVehicleOn(const CanInput& can)
  {
    return can.vehicle_state == VehicleState::Standby || can.vehicle_state == VehicleState::Drive;
  }

  static bool isVehicleInDrive(const CanInput& can) { return can.vehicle_state == VehicleState::Drive; }

  static bool isGearD(Gear g) { return g == Gear::D; }
  static bool isGearN(Gear g) { return g == Gear::N; }
  static bool isGearP(Gear g) { return g == Gear::P; }

  static bool isIviOn(const InputSnapshot& in) { return in.ivi_tsr_on; }
  static bool isIviOff(const InputSnapshot& in) { return !in.ivi_tsr_on; }

  static bool isFailure(const InputSnapshot& in)
  {
    if (!isVehicleOn(in.can))
      return false;
    return !in.can.camera_ok || !in.can.speed_valid || !in.can.eps_valid || !in.can.acu_valid;
  }

  static bool isNoDisplay(const InputSnapshot& in, const TsrInternalContext& ctx, TimePoint now)
  {
    const auto elapsed = [&](TimePoint start, int ms) {
      if (start == TimePoint{})
        return false;
      return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > ms;
    };

    return (elapsed(ctx.yaw_turn_start, kYawTurnThresholdMs) && in.can.turning) ||
           (elapsed(ctx.yaw_u_turn_start, kYawUTurnThresholdMs) && in.can.speed_kmh < kMaxUTurnSpeed);
  }

  static void change(StateType& s, Level0 l0, Level1 l1 = Level1::None)
  {
    s.l0 = l0;
    s.l1 = l1;
  }

  static void processTopLevel(StateType& s, const InputSnapshot& in)
  {
    switch (s.l0) {
      case Level0::Init:
        if (!in.tsr_feature_on)
          change(s, Level0::Off);
        else if (isFailure(in))
          change(s, Level0::Failure);
        else
          change(s, Level0::Standby);
        break;

      case Level0::Off:
        if (isIviOn(in) && isFailure(in))
          change(s, Level0::Failure);
        else if (isVehicleOn(in.can) && isIviOn(in))
          change(s, Level0::Standby);
        break;

      case Level0::Failure:
        if (isIviOff(in))
          change(s, Level0::Off);
        else if (!isFailure(in))
          change(s, Level0::Standby);
        break;

      case Level0::Standby:
        if (isFailure(in))
          change(s, Level0::Failure);
        else if (isIviOff(in))
          change(s, Level0::Off);
        else if (isVehicleInDrive(in.can) && isGearD(in.can.gear))
          change(s, Level0::Active, Level1::NoDisplay);
        break;

      case Level0::Active:
        if (isFailure(in))
          change(s, Level0::Failure);
        else if (isIviOff(in))
          change(s, Level0::Off);
        break;
    }
  }

  static void processActiveSubstate(StateType& s, const InputSnapshot& in, const TsrInternalContext& ctx, TimePoint now)
  {
    switch (s.l1) {
      case Level1::NoDisplay:
        if (!isGearD(in.can.gear))
          change(s, Level0::Standby);
        else if (!isNoDisplay(in, ctx, now))
          change(s, Level0::Active, Level1::Display);
        break;

      case Level1::Display:
        if (!(isGearD(in.can.gear) || isGearN(in.can.gear) || isGearP(in.can.gear)))
          change(s, Level0::Standby);
        else if (isGearN(in.can.gear) || isGearP(in.can.gear))
          change(s, Level0::Active, Level1::ContinueDisplayingPermanent);
        else if (isNoDisplay(in, ctx, now))
          change(s, Level0::Active, Level1::NoDisplay);
        break;

      case Level1::ContinueDisplayingPermanent:
        if (!(isGearD(in.can.gear) || isGearN(in.can.gear) || isGearP(in.can.gear)))
          change(s, Level0::Standby);
        else if (isNoDisplay(in, ctx, now))
          change(s, Level0::Active, Level1::NoDisplay);
        else if (isGearD(in.can.gear))
          change(s, Level0::Active, Level1::Display);
        break;

      case Level1::None:
        break;
    }
  }

  static std::optional<SpeedLimitSign> mostImportantSign(const std::vector<SpeedLimitSign>& signs)
  {
    std::optional<SpeedLimitSign> best;
    for (const auto& sign : signs) {
      if (!best || sign.limit_kmh > best->limit_kmh)
        best = sign;
    }
    return best;
  }

  static void processSpeedLimits(TsrTickResult& result, TsrInternalContext& ctx, const InputSnapshot& in)
  {
    if (result.state.l0 != Level0::Active || result.state.l1 != Level1::Display)
      return;

    const auto sign = mostImportantSign(in.signs);
    if (!sign || sign->is_end_sign)
      return;

    if (ctx.last_displayed_limit && *ctx.last_displayed_limit == sign->limit_kmh)
      return;

    ctx.last_displayed_limit = sign->limit_kmh;
    result.messages.push_back(
        {"/assist/trafficSignRecognition/detectedSpeedLimitValue", std::to_string(sign->limit_kmh)});
  }

  static void pushStateMessages(TsrTickResult& result, StateType old)
  {
    result.messages.push_back({"/assist/trafficSignRecognition/typeState", stateName(result.state)});

    if (!old.isOn() && result.state.l0 == Level0::Standby)
      result.messages.push_back({"/assist/trafficSignRecognition/state", "ON"});
    if (old.l0 != Level0::Off && result.state.l0 == Level0::Off)
      result.messages.push_back({"/assist/trafficSignRecognition/state", "OFF"});
    if (old.l0 != Level0::Failure && result.state.l0 == Level0::Failure)
      result.messages.push_back({"/assist/trafficSignRecognition/state", "INVALID"});
  }
};

// =============================================================================
// APPLICATION
// =============================================================================

class IInputStore {
public:
  virtual ~IInputStore() = default;
  virtual void patch(std::function<void(InputSnapshot&)> fn) = 0;
  virtual InputSnapshot load() = 0;
};

class IOutputSink {
public:
  virtual ~IOutputSink() = default;
  virtual void emit(const TsrOutput& output) = 0;
};

class ProcessTsrTickUseCase {
public:
  ProcessTsrTickUseCase(IInputStore& inputs, IOutputSink& outputs) : inputs_(inputs), outputs_(outputs) {}

  void execute()
  {
    const auto result = machine_.tick(state_, ctx_, inputs_.load(), Clock::now());
    state_ = result.state;
    for (const auto& msg : result.messages)
      outputs_.emit(msg);
  }

  StateType state() const { return state_; }

private:
  IInputStore& inputs_;
  IOutputSink& outputs_;
  TsrStateMachine machine_;
  StateType state_;
  TsrInternalContext ctx_;
};

// =============================================================================
// INFRASTRUCTURE
// =============================================================================

class ThreadSafeInputStore : public IInputStore {
public:
  void patch(std::function<void(InputSnapshot&)> fn) override
  {
    std::lock_guard<std::mutex> lock(mutex_);
    fn(snapshot_);
  }

  InputSnapshot load() override
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_;
  }

private:
  std::mutex mutex_;
  InputSnapshot snapshot_;
};

class ConsoleOutputSink : public IOutputSink {
public:
  void emit(const TsrOutput& output) override
  {
    std::cout << "[publish] " << output.topic << " = " << output.payload << '\n';
  }
};

// =============================================================================
// ADAPTERS + COMPOSITION ROOT
// =============================================================================

struct SimCgw01 {
  VehicleState vehicle_state{VehicleState::Sleep};
};
struct SimVcuMcu03 {
  Gear gear{Gear::Unspecified};
};
struct SimIbsStatus03 {
  float speed{0};
  bool valid{false};
};
struct SimAcu01 {
  float yaw_rate{0};
  bool valid{true};
};
struct SimEpsConv {
  bool valid{true};
};
struct SimSwitches01 {
  bool turning{false};
};

class TsrNode {
public:
  TsrNode() : process_tick_(input_store_, output_sink_) {}

  void start()
  {
    std::cout << "--- scenario: Init -> Standby -> Active/Display ---\n";
    onTimer();  // Init resolves

    onCgw01(SimCgw01{VehicleState::Standby});
    onIviCommand(true);
    onIbsStatus03(SimIbsStatus03{0, true});
    onEps(SimEpsConv{true});
    onAcu(SimAcu01{0, true});
    onTimer();  // Off/Init -> Standby, publish ON

    onCgw01(SimCgw01{VehicleState::Drive});
    onVcu(SimVcuMcu03{Gear::D});
    onIbsStatus03(SimIbsStatus03{50, true});
    onTsrObjects({SpeedLimitSign{60, false, true}});
    onTimer();  // Standby -> Active/NoDisplay -> Active/Display + speed limit

    std::cout << "\n--- scenario: turn -> NoDisplay ---\n";
    onSwitches(SimSwitches01{true});
    onAcu(SimAcu01{15, true});
    onTimer();  // start yaw timer
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    onTimer();  // yaw duration exceeded -> NoDisplay

    std::cout << "\n--- scenario: N gear -> ContinueDisplayingPermanent ---\n";
    onSwitches(SimSwitches01{false});
    onAcu(SimAcu01{0, true});
    onTimer();  // NoDisplay -> Display (no yaw/turn)
    onVcu(SimVcuMcu03{Gear::N});
    onTimer();  // Display + N -> ContinueDisplayingPermanent

    std::cout << "\n--- scenario: camera fault -> Failure ---\n";
    onCameraFault();
    onTimer();
  }

  StateType state() const { return process_tick_.state(); }

private:
  void onCgw01(const SimCgw01& msg)
  {
    input_store_.patch([&](InputSnapshot& s) { s.can.vehicle_state = msg.vehicle_state; });
  }

  void onVcu(const SimVcuMcu03& msg)
  {
    input_store_.patch([&](InputSnapshot& s) { s.can.gear = msg.gear; });
  }

  void onIbsStatus03(const SimIbsStatus03& msg)
  {
    input_store_.patch([&](InputSnapshot& s) {
      s.can.speed_kmh = msg.speed;
      s.can.speed_valid = msg.valid;
    });
  }

  void onEps(const SimEpsConv& msg)
  {
    input_store_.patch([&](InputSnapshot& s) { s.can.eps_valid = msg.valid; });
  }

  void onAcu(const SimAcu01& msg)
  {
    input_store_.patch([&](InputSnapshot& s) {
      s.can.yaw_rate = msg.yaw_rate;
      s.can.acu_valid = msg.valid;
    });
  }

  void onSwitches(const SimSwitches01& msg)
  {
    input_store_.patch([&](InputSnapshot& s) { s.can.turning = msg.turning; });
  }

  void onCameraFault()
  {
    input_store_.patch([&](InputSnapshot& s) { s.can.camera_ok = false; });
  }

  void onIviCommand(bool enabled)
  {
    input_store_.patch([&](InputSnapshot& s) { s.ivi_tsr_on = enabled; });
  }

  void onTsrObjects(const std::vector<SpeedLimitSign>& signs)
  {
    input_store_.patch([&](InputSnapshot& s) { s.signs = signs; });
  }

  void onTimer() { process_tick_.execute(); }

  ThreadSafeInputStore input_store_;
  ConsoleOutputSink output_sink_;
  ProcessTsrTickUseCase process_tick_;
};

int main()
{
  std::cout << "TSR CA example with simplified real state logic\n";
  std::cout << "Adapter -> IInputStore -> ProcessTsrTickUseCase -> Domain -> IOutputSink\n\n";

  TsrNode node;
  node.start();

  const auto& s = node.state();
  std::cout << "\nFinal: " << TsrStateMachine::stateName(s) << '\n';
  return 0;
}
