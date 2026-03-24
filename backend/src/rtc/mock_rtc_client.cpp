#include "mock_rtc_client.hpp"

#include "job_id.hpp"

#include <nlohmann/json.hpp>

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
  if (dxf_line_count_) s.active_dxf_line_count = dxf_line_count_;
  if (dxf_source_name_) s.active_dxf_source_name = dxf_source_name_;
  if (correction_hint_) s.correction_file_hint = correction_hint_;
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
  dxf_line_count_.reset();
  dxf_source_name_.reset();
  return std::nullopt;
}

void MockRtcClient::disconnect() {
  std::lock_guard<std::mutex> lock(mutex_);
  state_ = State::Disconnected;
  latched_error_.reset();
  current_label_.clear();
  current_job_id_.clear();
  dxf_line_count_.reset();
  dxf_source_name_.reset();
  correction_hint_.reset();
}

std::variant<RtcStatus, RtcError> MockRtcClient::get_status() {
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
  dxf_line_count_.reset();
  dxf_source_name_.reset();
  state_ = State::Loaded;
  return current_job_id_;
}

std::optional<RtcError> MockRtcClient::load_dxf_job(const nlohmann::json& job_document) {
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
  if (!job_document.contains("entities") || !job_document["entities"].is_array() ||
      job_document["entities"].empty()) {
    return err("DXF_PARSE_ERROR", "DXF job has no entities");
  }
  if (!job_document.contains("line_count") || !job_document["line_count"].is_number_integer()) {
    return err("DXF_PARSE_ERROR", "DXF job missing line_count");
  }
  const auto n = job_document["line_count"].get<std::size_t>();
  if (n == 0) return err("DXF_PARSE_ERROR", "DXF job line_count is zero");
  dxf_line_count_ = n;
  dxf_source_name_ = job_document.value("source_name", "dxf");
  current_label_.clear();
  current_job_id_.clear();
  state_ = State::Loaded;
  return std::nullopt;
}

std::optional<RtcError> MockRtcClient::start_execution(std::uint32_t repeat_count) {
  std::lock_guard<std::mutex> lock(mutex_);
  (void)repeat_count;
  if (state_ == State::Disconnected) {
    return err("RTC_NOT_CONNECTED", "RTC session not established");
  }
  if (state_ == State::Running) {
    return err("RTC_ALREADY_RUNNING", "Execution already running");
  }
  if (state_ != State::Loaded) {
    return err("RTC_NOT_READY", "Load a job before start");
  }
  // Match Ethernet: session stays running until Stop (or disconnect). Immediate "list finished"
  // made the UI keep Stop disabled while real boards could still be busy.
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

std::optional<RtcError> MockRtcClient::load_correction_file(const std::vector<std::uint8_t>& file_bytes,
                                                            const CorrectionFileLoadParams& params) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (state_ == State::Disconnected) {
    return err("RTC_NOT_CONNECTED", "RTC session not established");
  }
  if (state_ == State::Running) {
    return err("RTC_BUSY", "Cannot load correction file while running");
  }
  if (state_ == State::Error) {
    return err("RTC_INTERNAL", "RTC in error state; disconnect or reset");
  }
  if (file_bytes.empty()) return err("RTC_INTERNAL", "correction file is empty");
  (void)params;
  correction_hint_ =
      "mock: " + std::to_string(file_bytes.size()) + " bytes (table " + std::to_string(params.table_no) + ")";
  return std::nullopt;
}

}  // namespace laserdesk::rtc
