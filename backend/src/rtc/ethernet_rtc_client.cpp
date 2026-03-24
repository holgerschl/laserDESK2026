#include "ethernet_rtc_client.hpp"

#include "job/dxf_rif_list_mapper.hpp"
#include "job/rtc_job_plan.hpp"
#include "job_id.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <thread>
#include <vector>

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

/// RTC6 answer payload: `[LastError, CmdId, …]`; `get_head_para` returns IEEE double in two `uint32_t` words
/// (same packing as `rtc6_rif_wrapper.cpp` `as_double`).
bool try_decode_double_return(const rif::ParsedAnswer& a, double& out) {
  if (a.pl_words.size() < 4u) return false;
  std::uint32_t parts[2] = {a.pl_words[2], a.pl_words[3]};
  std::memcpy(&out, parts, sizeof(out));
  return true;
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
  if (dxf_line_count_) s.active_dxf_line_count = dxf_line_count_;
  if (dxf_source_name_) s.active_dxf_source_name = dxf_source_name_;
  s.rif_udp_timeout_count = rif_metric_udp_timeouts_;
  s.rif_udp_spurious_datagrams = rif_metric_spurious_datagrams_;
  s.rif_connect_status_retries_used = rif_last_connect_status_retries_;
  return s;
}

std::optional<RtcError> EthernetRtcClient::send_remote_control(const std::vector<std::uint32_t>& words,
                                                                rif::ParsedAnswer& out) const {
  const std::uint32_t seq = ++seq_;
  std::vector<std::uint8_t> pkt = rif::build_command_telegram(seq, format_, words);
  std::vector<std::uint8_t> raw;
  std::uint64_t spurious = 0;
  std::string io_err = udp_.request_response_matching(pkt, timeout_ms_, seq, format_, max_extra_datagrams_, raw,
                                                      &spurious);
  rif_metric_spurious_datagrams_ += spurious;
  if (!io_err.empty()) {
    if (io_err == "UDP receive timeout") ++rif_metric_udp_timeouts_;
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
  timeout_ms_ = cfg.recv_timeout_ms > 0 ? cfg.recv_timeout_ms : kDefaultRtcUdpRecvTimeoutMs;
  max_extra_datagrams_ = std::clamp(cfg.rif_udp_max_extra_datagrams, 0, 64);
  connect_status_attempts_ = std::clamp(cfg.rif_connect_status_attempts, 1, 20);
  rif_retry_delay_ms_ = std::clamp(cfg.rif_retry_delay_ms, 0, 30000);
  package_tag_ = cfg.expected_package_tag;
  bios_tag_ = cfg.expected_bios_eth_tag;
  dxf_rif_list_upload_ = cfg.dxf_rif_list_upload;
  connect_default_bits_per_mm_ = cfg.dxf_rif_bits_per_mm > 0.0 ? cfg.dxf_rif_bits_per_mm : 128.0;
  dxf_rif_bits_per_mm_ = connect_default_bits_per_mm_;
  rif_config_list_mem1_ = cfg.rif_config_list_mem1;
  rif_config_list_mem2_ = cfg.rif_config_list_mem2;
  rif_metric_udp_timeouts_ = 0;
  rif_metric_spurious_datagrams_ = 0;
  rif_last_connect_status_retries_ = 0;

  std::string oerr = udp_.open(cfg.host, static_cast<std::uint16_t>(cfg.port), cfg.udp_local_bind);
  if (!oerr.empty()) {
    return err("RTC_CONNECTION_REFUSED", oerr);
  }

  seq_ = 0;
  // rtc6_rif_wrapper.cpp RTC(): send_recv({0x12345678}) with header seqnum 0, then
  // seqnum = answ.payload.buffer[0] + 1. Align before GET_STATUS so we follow the board counter.
  {
    constexpr std::uint32_t kSyncHdrSeq = 0u;
    std::vector<std::uint8_t> sync_pkt =
        rif::build_command_telegram(kSyncHdrSeq, format_, {rif::kRifSeqSyncPayload});
    std::vector<std::uint8_t> raw_sync;
    if (udp_.request_response(sync_pkt, timeout_ms_, raw_sync).empty()) {
      rif::ParsedAnswer sync_ans =
          rif::parse_answer_telegram(raw_sync.data(), raw_sync.size(), kSyncHdrSeq, format_);
      std::uint32_t last_board = 0;
      if (rif::try_parse_seq_sync_answer(sync_ans, last_board)) seq_ = last_board;
    }
  }

  rif::ParsedAnswer ans;
  std::optional<RtcError> connect_err;
  for (int attempt = 0; attempt < connect_status_attempts_; ++attempt) {
    if (attempt > 0) {
      ++rif_last_connect_status_retries_;
      if (rif_retry_delay_ms_ > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(rif_retry_delay_ms_));
      }
    }
    connect_err = send_remote_control({rif::kRdcGetStatus}, ans);
    if (!connect_err) {
      connect_err = check_answer(ans, rif::kRdcGetStatus, 4u);
    }
    if (!connect_err) break;
    if (connect_err->code != "RTC_TIMEOUT" || attempt + 1 >= connect_status_attempts_) {
      udp_.close();
      return connect_err;
    }
  }

  state_ = State::ConnectedIdle;
  last_job_id_.clear();
  last_job_label_.clear();
  dxf_line_count_.reset();
  dxf_source_name_.reset();
  return std::nullopt;
}

void EthernetRtcClient::disconnect() {
  std::lock_guard<std::mutex> lock(mutex_);
  udp_.close();
  state_ = State::Disconnected;
  seq_ = 0;
  last_job_id_.clear();
  last_job_label_.clear();
  dxf_line_count_.reset();
  dxf_source_name_.reset();
  dxf_rif_list_upload_ = false;
  dxf_rif_bits_per_mm_ = connect_default_bits_per_mm_;
  rif_config_list_mem1_ = 1u;
  rif_config_list_mem2_ = 2u;
  rif_metric_udp_timeouts_ = 0;
  rif_metric_spurious_datagrams_ = 0;
  rif_last_connect_status_retries_ = 0;
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
  dxf_line_count_.reset();
  dxf_source_name_.reset();
  state_ = State::Loaded;
  return last_job_id_;
}

std::optional<RtcError> EthernetRtcClient::load_dxf_job(const nlohmann::json& job_document) {
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
  if (!job_document.contains("entities") || !job_document["entities"].is_array() ||
      job_document["entities"].empty()) {
    return err("DXF_PARSE_ERROR", "DXF job has no entities");
  }
  if (!job_document.contains("line_count") || !job_document["line_count"].is_number_integer()) {
    return err("DXF_PARSE_ERROR", "DXF job missing line_count");
  }
  const auto n = job_document["line_count"].get<std::size_t>();
  if (n == 0) return err("DXF_PARSE_ERROR", "DXF job line_count is zero");

  rif::ParsedAnswer ans;

  if (dxf_rif_list_upload_) {
    if (auto e = send_remote_control(
            {rif::kRdcConfigList, rif_config_list_mem1_, rif_config_list_mem2_}, ans)) {
      return e;
    }
    if (auto e = check_answer(ans, rif::kRdcConfigList, 2u)) {
      return e;
    }
  }

  if (auto e = send_remote_control({rif::kRdcGetInputPointer}, ans)) {
    return e;
  }
  if (auto e = check_answer(ans, rif::kRdcGetInputPointer, 3u)) {
    return e;
  }

  if (dxf_rif_list_upload_) {
    job::RtcJobPlan plan;
    std::string perr;
    if (!job::parse_rtc_job_plan_from_dxf_json(job_document, plan, perr)) {
      return err("DXF_PARSE_ERROR", perr);
    }
    job::DxfRifListMapParams mp;
    mp.bits_per_mm = dxf_rif_bits_per_mm_;
    std::vector<std::vector<std::uint32_t>> seq;
    if (!job::build_dxf_rif_list_upload_sequence(plan, mp, seq, perr)) {
      return err("RTC_INTERNAL", perr);
    }
    for (const auto& words : seq) {
      if (words.empty()) {
        return err("RTC_INTERNAL", "empty RIF list command");
      }
      if (auto e = send_remote_control(words, ans)) {
        return e;
      }
      if (auto e = check_answer(ans, words[0], 2u)) {
        return e;
      }
    }
  }

  dxf_line_count_ = n;
  dxf_source_name_ = job_document.value("source_name", "dxf");
  last_job_label_.clear();
  last_job_id_.clear();
  state_ = State::Loaded;
  return std::nullopt;
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

std::optional<RtcError> EthernetRtcClient::load_correction_file(const std::vector<std::uint8_t>& file_bytes,
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

  rif::ParsedAnswer ans;
  if (params.number_of_tables.has_value()) {
    if (auto e = send_remote_control({rif::kRdcNumberOfCorTables, *params.number_of_tables}, ans)) {
      return e;
    }
    if (auto e = check_answer(ans, rif::kRdcNumberOfCorTables, 2u)) return e;
  }

  // rtc6_rif_wrapper.cpp load_correction_file — chunk size matches TGM payload minus 3 header words.
  constexpr std::uint32_t kPayloadMax = rif::kTgmMaxTelegramBytes - rif::kTgmHeaderBytes;
  constexpr std::uint32_t kMaxCorrData = kPayloadMax - 3u * static_cast<std::uint32_t>(sizeof(std::uint32_t));

  for (std::uint32_t offset = 0; offset < file_bytes.size();) {
    const std::uint32_t rem = static_cast<std::uint32_t>(file_bytes.size() - offset);
    std::uint32_t chunk = (std::min)(rem, kMaxCorrData);
    chunk = (chunk / 4u) * 4u;
    if (chunk == 0u) chunk = rem;

    const std::size_t n_words = (static_cast<std::size_t>(chunk) + 3u) / 4u;
    std::vector<std::uint32_t> words(3u + n_words);
    words[0] = rif::kRdcLoadCorrectionFile;
    words[1] = offset;
    words[2] = chunk;
    std::memcpy(words.data() + 3, file_bytes.data() + offset, chunk);

    if (auto e = send_remote_control(words, ans)) return e;
    if (auto e = check_answer(ans, rif::kRdcLoadCorrectionFile, 2u)) return e;
    offset += chunk;
  }

  constexpr std::uint32_t kFinalizeOffset = std::numeric_limits<std::uint32_t>::max();
  const std::uint32_t no_dim =
      (params.table_no & 0xFFFFu) | ((params.dim & 0xFFFFu) << 16u);
  if (auto e = send_remote_control({rif::kRdcLoadCorrectionFile, kFinalizeOffset, no_dim}, ans)) {
    return e;
  }
  if (auto e = check_answer(ans, rif::kRdcLoadCorrectionFile, 2u)) return e;

  if (auto e = send_remote_control({rif::kRdcSelectCorTable, params.head_a, params.head_b}, ans)) {
    return e;
  }
  if (auto e = check_answer(ans, rif::kRdcSelectCorTable, 2u)) return e;

  // Manual Ch. 10 p. 465: `get_head_para(HeadNo, ParaNo)` — ParaNo 1 = K xy [bit/mm] (ct5 header, p. 191).
  std::vector<std::uint32_t> head_nos;
  if (params.head_a != 0u) head_nos.push_back(1u);
  if (params.head_b != 0u) head_nos.push_back(2u);
  if (head_nos.empty()) {
    head_nos.push_back(1u);
    head_nos.push_back(2u);
  }
  constexpr std::uint32_t kParaKxyBitsPerMm = 1u;
  for (std::uint32_t head_no : head_nos) {
    if (auto e = send_remote_control({rif::kRdcGetHeadPara, head_no, kParaKxyBitsPerMm}, ans)) {
      (void)e;
      continue;
    }
    if (check_answer(ans, rif::kRdcGetHeadPara, 4u)) continue;
    double k = 0.0;
    if (!try_decode_double_return(ans, k)) continue;
    if (k > 0.0 && std::isfinite(k)) {
      dxf_rif_bits_per_mm_ = k;
      break;
    }
  }

  return std::nullopt;
}

}  // namespace laserdesk::rtc
