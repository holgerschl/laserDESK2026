#pragma once

#include "rtc_client.hpp"
#include "rif/telegram_raw.hpp"
#include "rif/udp_channel.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace laserdesk::rtc {

class EthernetRtcClient final : public IRtcClient {
 public:
  EthernetRtcClient() = default;

  std::optional<RtcError> connect(const RtcConnectConfig& cfg) override;
  void disconnect() override;
  std::variant<RtcStatus, RtcError> get_status() override;
  std::variant<std::string, RtcError> load_minimal_job(const std::string& label) override;
  std::optional<RtcError> load_dxf_job(const nlohmann::json& job_document) override;
  std::optional<RtcError> start_execution(std::uint32_t repeat_count = 1u) override;
  std::optional<RtcError> stop_execution() override;
  std::optional<RtcError> load_correction_file(const std::vector<std::uint8_t>& file_bytes,
                                               const CorrectionFileLoadParams& params) override;

 private:
  enum class State { Disconnected, ConnectedIdle, Loaded, Running, Error };

  static RtcError err(std::string code, std::string message);
  RtcStatus build_status(const rif::ParsedAnswer* get_status_answer) const;

  std::optional<RtcError> send_remote_control(const std::vector<std::uint32_t>& words,
                                              rif::ParsedAnswer& out) const;

  mutable std::mutex mutex_;
  rif::UdpRifChannel udp_;
  State state_{State::Disconnected};
  /// Incremented on each UDP telegram (`get_status()` may send).
  mutable std::uint32_t seq_{0};
  std::uint32_t format_{0};
  int timeout_ms_{kDefaultRtcUdpRecvTimeoutMs};
  int max_extra_datagrams_{kDefaultRifUdpMaxExtraDatagrams};
  int connect_status_attempts_{kDefaultRifConnectStatusAttempts};
  int rif_retry_delay_ms_{kDefaultRifRetryDelayMs};
  mutable std::uint64_t rif_metric_udp_timeouts_{0};
  mutable std::uint64_t rif_metric_spurious_datagrams_{0};
  std::uint32_t rif_last_connect_status_retries_{0};
  std::string package_tag_;
  std::string bios_tag_;
  std::string last_job_label_;
  std::string last_job_id_;
  std::optional<std::size_t> dxf_line_count_;
  std::optional<std::string> dxf_source_name_;
  /// After `load_dxf_job`: first word of `R_DC_GET_INPUT_POINTER` answer (payload `pl_words[2]`) — list
  /// memory index where streamed `R_LC_*` commands begin; must be passed to `R_DC_EXECUTE_LIST_POS` pos arg.
  std::optional<std::uint32_t> dxf_list_execute_start_pos_;
  bool dxf_rif_list_upload_{false};
  /// From `POST /rtc/connect` (`dxf_rif_bits_per_mm`); restored on disconnect; baseline if correction K_xy is 0.
  double connect_default_bits_per_mm_{128.0};
  double dxf_rif_bits_per_mm_{128.0};
  std::uint32_t rif_config_list_mem1_{1u};
  std::uint32_t rif_config_list_mem2_{2u};
  /// First argument to `R_DC_EXECUTE_LIST_POS` (see `RtcConnectConfig::rif_execute_list_no`).
  std::uint32_t rif_execute_list_no_{1u};
  /// From last `load_dxf_job` `"laser"` speeds (**mm/s**); with connect defaults converted to RIF **bits/ms**
  /// via K (bits/mm) at `start_execution`. Clear on disconnect / minimal load.
  std::optional<double> loaded_job_jump_speed_mm_s_;
  std::optional<double> loaded_job_mark_speed_mm_s_;
  /// Last `repeat_count` passed to `start_execution` (API / `R_DC_SET_MAX_COUNT`). See `get_status` idle FSM.
  std::uint32_t execution_repeat_count_{1u};
  /// Set while **Running** when `R_DC_GET_STATUS` lists list-execution **busy** at least once for this start.
  /// Auto-`Loaded` only after `saw_busy` then idle so we never flip on “idle before execution”.
  bool execution_saw_busy_{false};
  double rif_jump_speed_mm_s_{2000.0};
  double rif_mark_speed_mm_s_{250.0};
};

}  // namespace laserdesk::rtc
