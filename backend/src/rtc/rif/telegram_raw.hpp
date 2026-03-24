#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace laserdesk::rtc::rif {

/// RTC6 manual §16.10.6 — TGM_FORMAT RAW, TGM_VERSION 0x01000000 (telegrams.h).
constexpr std::uint32_t kTgmHeaderBytes = 20;
constexpr std::uint32_t kTgmVersion01000000 = 0x01000000u;
constexpr std::uint32_t kTypeCommand = 0u;
constexpr std::uint32_t kTypeAnswer = 1u;

/// `remote_interface::TGM_FORMAT::RAW` in SCANLAB `telegrams.h` (wrapper always uses RAW).
constexpr std::uint32_t kTgmFormatRaw = 1u;

/// Remote Control Command IDs (manual §16.10.9; numeric values match SCANLAB package `telegrams.h`).
constexpr std::uint32_t kRdcConfigList = 1u;
/// `set_start_list_pos(ListNo, Pos)` — Appendix A ID 3; aligns list write/start address before `load_list` (wrapper §6.4.1).
constexpr std::uint32_t kRdcSetStartListPos = 3u;
constexpr std::uint32_t kRdcGetInputPointer = 4u;
/// `load_list(ListNo, Pos)` — manual §16.10.9 Appendix A; initializes protected list loading (before `R_LC_*` stream).
constexpr std::uint32_t kRdcLoadListPos = 6u;
/// Remote control ID 11 — maps to DLL `set_max_counts`. RTC6 manual Ch.10 defines this for **External Starts**
/// (and `get_counts`), not proven here as “repeat this list N times”; see `EthernetRtcClient::start_execution`.
constexpr std::uint32_t kRdcSetMaxCount = 11u;
constexpr std::uint32_t kRdcExecuteListPos = 15u;
constexpr std::uint32_t kRdcStopExecution = 16u;
constexpr std::uint32_t kRdcGetStatus = 31u;
/// `get_rtc_version()` / `get_bios_version()` — template `RIF_test.cpp` RI path queries these after connect.
constexpr std::uint32_t kRdcGetRtcVersion = 142u;
constexpr std::uint32_t kRdcGetBiosVersion = 143u;
/// Manual Ch. 10 / `telegrams.h` — `get_head_para(HeadNo, ParaNo)`; ParaNo 1 = K xy [bit/mm] (ct5 header).
constexpr std::uint32_t kRdcGetHeadPara = 38u;
constexpr std::uint32_t kRdcSelectCorTable = 130u;
constexpr std::uint32_t kRdcLoadCorrectionFile = 154u;
constexpr std::uint32_t kRdcNumberOfCorTables = 155u;

/// SCANLAB `telegrams.h` `TGM_MAX_SIZE` (1400): max **total** Command/Answer telegram size in bytes (header + payload).
constexpr std::uint32_t kTgmMaxTelegramBytes = 1400u;

/// Manual §16.10.11 / `rtc6_rif_wrapper.cpp` RTC ctor — sole payload word for seq counter query.
constexpr std::uint32_t kRifSeqSyncPayload = 0x12345678u;

struct ParsedAnswer {
  bool ok{false};
  std::string parse_error;
  std::uint32_t seqnum{0};
  std::uint32_t last_error{0};
  /// Full TGM_PL_ANSW_RAW as little-endian uint32 words.
  std::vector<std::uint32_t> pl_words;
};

/// Build one Command Telegram: TGM_HEADER + TGM_PL_CMD_RAW (uint32 little-endian words).
std::vector<std::uint8_t> build_command_telegram(std::uint32_t seq, std::uint32_t format,
                                                 const std::vector<std::uint32_t>& cmd_payload_words);

/// Parse Answer Telegram; `expect_seq` must match header.seqnum.
ParsedAnswer parse_answer_telegram(const std::uint8_t* data, std::size_t len,
                                   std::uint32_t expect_seq, std::uint32_t expect_format);

/// True if `data` looks like a valid RAW Answer telegram for `expect_format` with header seq
/// `expect_seq` (header + payload length sanity only; does not validate LastError/CmdID).
bool answer_raw_matches_seq_and_format(const std::uint8_t* data, std::size_t len,
                                       std::uint32_t expect_seq, std::uint32_t expect_format);

/// Answer to seq sync: `rtc6_rif_wrapper.cpp` uses `answ.payload.buffer[0]` as last board seqnum
/// (not a normal LastError/CmdID answer). `ParsedAnswer::last_error` mirrors word[0] and is not
/// a TGM_ERROR here — do not use `describe_last_error` on sync replies.
bool try_parse_seq_sync_answer(const ParsedAnswer& parsed, std::uint32_t& last_seq_out);

/// Human-readable hint for LastError low bits (manual §16.10.7).
std::string describe_last_error(std::uint32_t last_error);

/// True if LastError indicates invalid telegram (Bit #31 in Remote Interface context).
bool last_error_invalid_telegram(std::uint32_t last_error);

}  // namespace laserdesk::rtc::rif
