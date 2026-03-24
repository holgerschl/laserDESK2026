#pragma once

#include <cstdint>

namespace laserdesk::rtc::rif {

/// List command IDs (`enum list_command` in SCANLAB RTC6 software package `telegrams.h`).
/// Used as the first `uint32_t` in a RAW command telegram payload (one list command per UDP datagram).
constexpr std::uint32_t kLcEndOfList = 259u;
/// SCANLAB `Remote Interface/telegrams.h` — same `DOUBLE_PARA` **bits/ms** as `R_DC_SET_MARK/JUMP_SPEED`.
constexpr std::uint32_t kLcSetMarkSpeed = 301u;
constexpr std::uint32_t kLcSetJumpSpeed = 302u;
constexpr std::uint32_t kLcMarkXyztAbs = 312u;
constexpr std::uint32_t kLcJumpXyAbs = 321u;

}  // namespace laserdesk::rtc::rif
