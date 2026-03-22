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

/// Remote Control Command IDs (manual §16.10.9; numeric values match SCANLAB package `telegrams.h`).
constexpr std::uint32_t kRdcConfigList = 1u;
constexpr std::uint32_t kRdcGetInputPointer = 4u;
constexpr std::uint32_t kRdcExecuteListPos = 15u;
constexpr std::uint32_t kRdcStopExecution = 16u;
constexpr std::uint32_t kRdcGetStatus = 31u;

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

/// Human-readable hint for LastError low bits (manual §16.10.7).
std::string describe_last_error(std::uint32_t last_error);

/// True if LastError indicates invalid telegram (Bit #31 in Remote Interface context).
bool last_error_invalid_telegram(std::uint32_t last_error);

}  // namespace laserdesk::rtc::rif
