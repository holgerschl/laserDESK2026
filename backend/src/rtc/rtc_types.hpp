#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace laserdesk::rtc {

/// Default per-datagram UDP receive timeout for RTC6 Ethernet Remote Interface (ms).
inline constexpr int kDefaultRtcUdpRecvTimeoutMs = 3000;
/// Total attempts for idempotent `R_DC_GET_STATUS` during Ethernet connect (includes first try).
inline constexpr int kDefaultRifConnectStatusAttempts = 3;
/// Max additional UDP datagrams read while waiting for matching answer seq (reordering cap).
inline constexpr int kDefaultRifUdpMaxExtraDatagrams = 8;
/// Delay before retrying connect-time GET_STATUS after timeout (ms).
inline constexpr int kDefaultRifRetryDelayMs = 50;

struct RtcError {
  std::string code;
  std::string message;
};

struct RtcConnectConfig {
  enum class Mode { Mock, Ethernet };
  Mode mode{Mode::Mock};
  /// RTC6 Ethernet board hostname or IP (Remote Interface / UDP).
  std::string host;
  /// UDP port (exclusive RIF port on board — confirm with eth_get_port_numbers / RTC6conf; default 63750).
  int port{63750};
  /// TGM header `format`; must match `eth_set_remote_tgm_format` (SCANLAB `telegrams.h`: RAW = 1).
  std::uint32_t tgm_format{1u};
  int recv_timeout_ms{kDefaultRtcUdpRecvTimeoutMs};
  /// If non-empty, bind the local UDP socket to this host (IPv4); default ephemeral on any interface.
  std::string udp_local_bind;
  /// Total attempts for `R_DC_GET_STATUS` during connect after seq sync (idempotent; default 3).
  int rif_connect_status_attempts{kDefaultRifConnectStatusAttempts};
  /// Per-request cap on extra datagrams when matching answer seq (manual reordering note).
  int rif_udp_max_extra_datagrams{kDefaultRifUdpMaxExtraDatagrams};
  /// Pause between connect GET_STATUS retries after timeout (ms).
  int rif_retry_delay_ms{kDefaultRifRetryDelayMs};
  /// Shown in GET /rtc/status when set (operator-supplied; board identity query is future work).
  std::string expected_package_tag;
  std::string expected_bios_eth_tag;

  /// Phase G.4: after `R_DC_CONFIG_LIST` + `R_DC_GET_INPUT_POINTER`, stream `R_LC_*` list commands
  /// (see package `telegrams.h` / `rif/remote_list_commands.hpp`). REST `POST /rtc/connect` with
  /// `mode: ethernet` sets this **true** unless the body sets `dxf_rif_list_upload: false`.
  bool dxf_rif_list_upload{false};
  /// Scales DXF coordinates to scanner units (see `dxf_rif_list_mapper.cpp`).
  double dxf_rif_bits_per_mm{128.0};
  /// Arguments to `R_DC_CONFIG_LIST` (1) — same as SCANLAB Remote Interface `config_list`.
  std::uint32_t rif_config_list_mem1{1u};
  std::uint32_t rif_config_list_mem2{2u};
  /// First word of `R_DC_EXECUTE_LIST_POS` (15) — which list to run. If unset at REST connect, defaults to
  /// `rif_config_list_mem1` (same list targeted by `config_list` on many setups).
  std::optional<std::uint32_t> rif_execute_list_no;
  /// Desired **image-plane** jump/mark speeds in **mm/s** (scene/job/UI); before execute we convert to the
  /// RIF ctrl payload **bits/ms** using RTC6 manual v(m/s)=Speed/K ⇒ Speed=v×K with K=**bits/mm** from
  /// correction (`get_head_para` / `dxf_rif_bits_per_mm`). Defaults match `laserProperties.ts`. **≤ 0** skips.
  double rif_jump_speed_mm_s{2000.0};
  double rif_mark_speed_mm_s{250.0};
};

/// `R_DC_LOAD_CORRECTION_FILE` + `R_DC_SELECT_COR_TABLE` (see `rtc6_rif_wrapper.cpp` / `telegrams.h`).
struct CorrectionFileLoadParams {
  /// RTC6 manual `load_correction_file` parameter **No**: allowed **[1…8]** (not 0). Packed into finalize
  /// telegram word 3 (low 16 bits) per §16.10.6 note (4).
  std::uint32_t table_no{1};
  std::uint32_t dim{2};
  std::uint32_t head_a{1};
  std::uint32_t head_b{1};
  /// If set, sends `R_DC_NUMBER_OF_COR_TABLES` before upload. If unset, Ethernet sends 1 (some firmware
  /// requires this before the first `R_DC_LOAD_CORRECTION_FILE` data chunk).
  std::optional<std::uint32_t> number_of_tables;
  /// Override third word of **finalize** only (`Offset = UINT_MAX`): default `(table_no & 0xFFFF) | (dim << 16)`
  /// per manual §16.10.6 (4). Normally leave unset.
  std::optional<std::uint32_t> finalize_arg3;
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
  /// Set when a DXF-derived job is loaded (Phase G).
  std::optional<std::size_t> active_dxf_line_count;
  std::optional<std::string> active_dxf_source_name;
  /// Mock: last loaded correction label; Ethernet: omitted (board holds table).
  std::optional<std::string> correction_file_hint;
  /// Ethernet session: UDP receive timeouts observed (cumulative since connect).
  std::optional<std::uint64_t> rif_udp_timeout_count;
  /// Ethernet: datagrams from the board skipped while waiting for matching answer seq.
  std::optional<std::uint64_t> rif_udp_spurious_datagrams;
  /// Ethernet: extra `R_DC_GET_STATUS` attempts during last connect after the first try (retries only).
  std::optional<std::uint32_t> rif_connect_status_retries_used;
  /// Ethernet: `R_DC_EXECUTE_LIST_POS` list index in use for this session (`POST /rtc/connect`).
  std::optional<std::uint32_t> rif_execute_list_no;
};

inline const char* kHealthOk = "ok";

}  // namespace laserdesk::rtc
