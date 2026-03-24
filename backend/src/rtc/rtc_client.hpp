#pragma once

#include "rtc_types.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace laserdesk::rtc {

class IRtcClient {
 public:
  virtual ~IRtcClient() = default;

  virtual std::optional<RtcError> connect(const RtcConnectConfig& cfg) = 0;
  virtual void disconnect() = 0;

  /// RTC_NOT_CONNECTED if not connected. May advance Ethernet session state (Running→Loaded) when idle.
  virtual std::variant<RtcStatus, RtcError> get_status() = 0;

  virtual std::variant<std::string, RtcError> load_minimal_job(const std::string& label) = 0;
  /// Load parsed DXF job document (same shape as GET /jobs/dxf/{id}) into the RTC session.
  virtual std::optional<RtcError> load_dxf_job(const nlohmann::json& job_document) = 0;
  /// `repeat_count` passed to `R_DC_SET_MAX_COUNT` before `R_DC_EXECUTE_LIST_POS`. RTC6 Ch.10 describes
  /// `set_max_counts` primarily for External Starts; Ethernet auto-idle (Running→Loaded) only when this is 1.
  virtual std::optional<RtcError> start_execution(std::uint32_t repeat_count = 1u) = 0;
  virtual std::optional<RtcError> stop_execution() = 0;

  /// Upload `.ct5` / correction blob via Remote Interface (Ethernet); no-op success on mock.
  virtual std::optional<RtcError> load_correction_file(const std::vector<std::uint8_t>& file_bytes,
                                                      const CorrectionFileLoadParams& params) = 0;

  /// Ethernet: recent outbound RIF command lines (mock: empty). Used by `GET /api/v1/rtc/rif_log`.
  virtual std::vector<std::string> snapshot_rif_command_log() const { return {}; }
  /// `mock` | `ethernet` — for API metadata without calling `get_status()` (no extra RIF traffic).
  virtual std::string rif_session_mode_label() const { return "unknown"; }
};

std::unique_ptr<IRtcClient> make_mock_rtc_client();
std::unique_ptr<IRtcClient> make_ethernet_rtc_client();

}  // namespace laserdesk::rtc
