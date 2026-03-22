#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace laserdesk::rtc {

struct RtcError {
  std::string code;
  std::string message;
};

struct RtcConnectConfig {
  enum class Mode { Mock, Ethernet };
  Mode mode{Mode::Mock};
  /// RTC6 Ethernet board hostname or IP (Remote Interface / UDP).
  std::string host;
  /// UDP port (same as conventional DLL transmission; default 5020 — confirm with eth_get_port_numbers).
  int port{5020};
  /// TGM format field; must match eth_set_remote_tgm_format on the board (RAW is typical).
  std::uint32_t tgm_format{0};
  int recv_timeout_ms{800};
  /// Shown in GET /rtc/status when set (operator-supplied; board identity query is future work).
  std::string expected_package_tag;
  std::string expected_bios_eth_tag;
};

/// Mirrors OpenAPI RtcStatusResponse / mock-rtc-specification.md
struct RtcStatus {
  std::string connection_state;  // disconnected | connected_idle | loaded | running | error
  std::string rtc_mode;          // mock | ethernet
  std::optional<std::string> package_version_reported;
  std::optional<std::string> bios_eth_reported;
  bool alignment_ok{true};
  std::optional<RtcError> last_error;
  /// Last successful R_DC_GET_STATUS (31) values when rtc_mode == ethernet.
  std::optional<std::uint32_t> remote_status_register;
  std::optional<std::uint32_t> remote_pos_register;
};

inline const char* kHealthOk = "ok";

}  // namespace laserdesk::rtc
