#include "ethernet_rtc_client.hpp"

#include "job_id.hpp"

namespace laserdesk::rtc {

namespace {

std::optional<RtcError> check_answer(rif::ParsedAnswer& a, std::uint32_t expected_cmd_id,
                                     std::size_t min_words) {
  if (rif::last_error_invalid_telegram(a.last_error)) {
    return RtcError{"RTC_INTERNAL", rif::describe_last_error(a.last_error)};
  }
  if (a.pl_words.size() < min_words) {
    return RtcError{"RTC_INTERNAL", "Answer payload too short"};
  }
  if (a.pl_words[1] != expected_cmd_id) {
    return RtcError{"RTC_INTERNAL", "Answer command ID mismatch"};
  }
  if (a.last_error != 0u) {
    return RtcError{"RTC_INTERNAL", rif::describe_last_error(a.last_error)};
  }
  return std::nullopt;
}

}  // namespace

std::unique_ptr<IRtcClient> make_ethernet_rtc_client() {
  return std::make_unique<EthernetRtcClient>();
}

RtcError EthernetRtcClient::err(std::string code, std::string message) {
  return RtcError{std::move(code), std::move(message)};
}

RtcStatus EthernetRtcClient::build_status(const rif::ParsedAnswer* g) const {
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
  s.rtc_mode = "ethernet";
  if (!package_tag_.empty()) s.package_version_reported = package_tag_;
  if (!bios_tag_.empty()) s.bios_eth_reported = bios_tag_;
  s.alignment_ok = true;
  if (g && g->pl_words.size() >= 4u) {
    s.remote_status_register = g->pl_words[2];
    s.remote_pos_register = g->pl_words[3];
  }
  return s;
}

std::optional<RtcError> EthernetRtcClient::send_remote_control(const std::vector<std::uint32_t>& words,
                                                                rif::ParsedAnswer& out) const {
  const std::uint32_t seq = ++seq_;
  std::vector<std::uint8_t> pkt = rif::build_command_telegram(seq, format_, words);
  std::vector<std::uint8_t> raw;
  std::string io_err = udp_.request_response(pkt, timeout_ms_, raw);
  if (!io_err.empty()) {
    return err("RTC_TIMEOUT", io_err);
  }
  out = rif::parse_answer_telegram(raw.data(), raw.size(), seq, format_);
  if (!out.ok) {
    return err("RTC_INTERNAL", out.parse_error);
  }
  return std::nullopt;
}

std::optional<RtcError> EthernetRtcClient::connect(const RtcConnectConfig& cfg) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (cfg.mode != RtcConnectConfig::Mode::Ethernet) {
    return err("RTC_INTERNAL", "EthernetRtcClient requires mode ethernet");
  }
  if (cfg.host.empty()) {
    return err("RTC_INTERNAL", "host is required for ethernet mode");
  }
  if (cfg.port <= 0 || cfg.port > 65535) {
    return err("RTC_INTERNAL", "invalid UDP port");
  }
  format_ = cfg.tgm_format;
  timeout_ms_ = cfg.recv_timeout_ms > 0 ? cfg.recv_timeout_ms : 800;
  package_tag_ = cfg.expected_package_tag;
  bios_tag_ = cfg.expected_bios_eth_tag;

  std::string oerr = udp_.open(cfg.host, static_cast<std::uint16_t>(cfg.port));
  if (!oerr.empty()) {
    return err("RTC_CONNECTION_REFUSED", oerr);
  }

  rif::ParsedAnswer ans;
  if (auto e = send_remote_control({rif::kRdcGetStatus}, ans)) {
    udp_.close();
    return e;
  }
  if (auto e = check_answer(ans, rif::kRdcGetStatus, 4u)) {
    udp_.close();
    return e;
  }

  state_ = State::ConnectedIdle;
  last_job_id_.clear();
  last_job_label_.clear();
  return std::nullopt;
}

void EthernetRtcClient::disconnect() {
  std::lock_guard<std::mutex> lock(mutex_);
  udp_.close();
  state_ = State::Disconnected;
  seq_ = 0;
  last_job_id_.clear();
  last_job_label_.clear();
}

std::variant<RtcStatus, RtcError> EthernetRtcClient::get_status() const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (state_ == State::Disconnected) {
    return err("RTC_NOT_CONNECTED", "RTC session not established");
  }
  rif::ParsedAnswer ans;
  if (auto e = send_remote_control({rif::kRdcGetStatus}, ans)) {
    return *e;
  }
  if (auto e = check_answer(ans, rif::kRdcGetStatus, 4u)) {
    return *e;
  }
  return build_status(&ans);
}

std::variant<std::string, RtcError> EthernetRtcClient::load_minimal_job(const std::string& label) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (state_ == State::Disconnected) {
    return err("RTC_NOT_CONNECTED", "RTC session not established");
  }
  if (state_ == State::Running) {
    return err("RTC_BUSY", "Cannot load job while running");
  }
  if (state_ == State::Error) {
    return err("RTC_INTERNAL", "RTC in error state; disconnect");
  }

  rif::ParsedAnswer ans;
  if (auto e = send_remote_control({rif::kRdcGetInputPointer}, ans)) {
    return *e;
  }
  if (auto e = check_answer(ans, rif::kRdcGetInputPointer, 3u)) {
    return *e;
  }

  last_job_label_ = label.empty() ? "phase-a-demo" : label;
  last_job_id_ = make_demo_job_id();
  state_ = State::Loaded;
  return last_job_id_;
}

std::optional<RtcError> EthernetRtcClient::start_execution() {
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
  rif::ParsedAnswer ans;
  if (auto e = send_remote_control({rif::kRdcExecuteListPos, 1u, 0u}, ans)) {
    return e;
  }
  if (auto e = check_answer(ans, rif::kRdcExecuteListPos, 2u)) {
    return e;
  }
  state_ = State::Running;
  return std::nullopt;
}

std::optional<RtcError> EthernetRtcClient::stop_execution() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (state_ == State::Disconnected) {
    return err("RTC_NOT_CONNECTED", "RTC session not established");
  }
  if (state_ != State::Running) {
    return err("RTC_NOT_RUNNING", "Execution is not running");
  }
  rif::ParsedAnswer ans;
  if (auto e = send_remote_control({rif::kRdcStopExecution}, ans)) {
    return e;
  }
  if (auto e = check_answer(ans, rif::kRdcStopExecution, 2u)) {
    return e;
  }
  state_ = State::Loaded;
  return std::nullopt;
}

}  // namespace laserdesk::rtc
