#include "mock_rtc_client.hpp"

#include "job_id.hpp"

namespace laserdesk::rtc {

std::unique_ptr<IRtcClient> make_mock_rtc_client() {
  return std::make_unique<MockRtcClient>();
}

RtcError MockRtcClient::err(std::string code, std::string message) {
  return RtcError{std::move(code), std::move(message)};
}

RtcStatus MockRtcClient::build_status() const {
  RtcStatus s;
  switch (state_) {
    case State::Disconnected:
      s.connection_state = "disconnected";
      break;
    case State::ConnectedIdle:
      s.connection_state = "connected_idle";
      break;
    case State::Loaded:
      s.connection_state = "loaded";
      break;
    case State::Running:
      s.connection_state = "running";
      break;
    case State::Error:
      s.connection_state = "error";
      break;
  }
  s.rtc_mode = "mock";
  if (state_ != State::Disconnected) {
    s.package_version_reported = "1.22.0-mock";
    s.bios_eth_reported = "mock-bios-eth";
    s.alignment_ok = true;
  }
  if (latched_error_) s.last_error = latched_error_;
  return s;
}

std::optional<RtcError> MockRtcClient::connect(const RtcConnectConfig& cfg) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (cfg.mode != RtcConnectConfig::Mode::Mock) {
    return err("RTC_INTERNAL", "MockRtcClient only supports mode mock");
  }
  state_ = State::ConnectedIdle;
  latched_error_.reset();
  current_label_.clear();
  current_job_id_.clear();
  return std::nullopt;
}

void MockRtcClient::disconnect() {
  std::lock_guard<std::mutex> lock(mutex_);
  state_ = State::Disconnected;
  latched_error_.reset();
  current_label_.clear();
  current_job_id_.clear();
}

std::variant<RtcStatus, RtcError> MockRtcClient::get_status() const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (state_ == State::Disconnected) {
    return err("RTC_NOT_CONNECTED", "RTC session not established");
  }
  return build_status();
}

std::variant<std::string, RtcError> MockRtcClient::load_minimal_job(const std::string& label) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (state_ == State::Disconnected) {
    return err("RTC_NOT_CONNECTED", "RTC session not established");
  }
  if (state_ == State::Running) {
    return err("RTC_BUSY", "Cannot load job while running");
  }
  if (state_ == State::Error) {
    return err("RTC_INTERNAL", "RTC in error state; disconnect or reset");
  }
  current_label_ = label.empty() ? "phase-a-demo" : label;
  current_job_id_ = make_demo_job_id();
  state_ = State::Loaded;
  return current_job_id_;
}

std::optional<RtcError> MockRtcClient::start_execution() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (state_ == State::Disconnected) {
    return err("RTC_NOT_CONNECTED", "RTC session not established");
  }
  if (state_ == State::Running) {
    return err("RTC_ALREADY_RUNNING", "Execution already running");
  }
  if (state_ != State::Loaded) {
    return err("RTC_NOT_READY", "Load a job before start");
  }
  state_ = State::Running;
  return std::nullopt;
}

std::optional<RtcError> MockRtcClient::stop_execution() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (state_ == State::Disconnected) {
    return err("RTC_NOT_CONNECTED", "RTC session not established");
  }
  if (state_ != State::Running) {
    return err("RTC_NOT_RUNNING", "Execution is not running");
  }
  state_ = State::Loaded;
  return std::nullopt;
}

}  // namespace laserdesk::rtc
