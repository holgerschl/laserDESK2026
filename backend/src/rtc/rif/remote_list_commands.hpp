#pragma once

#include <cstdint>

namespace laserdesk::rtc::rif {

/// List command IDs (`enum list_command` in SCANLAB `docs/telegrams.h`, Copyright SCANLAB GmbH).
/// Used as the first `uint32_t` in a RAW command telegram payload (one list command per UDP datagram
/// in `rtc6_rif_wrapper.cpp` style).
constexpr std::uint32_t kLcEndOfList = 259u;
constexpr std::uint32_t kLcMarkXyztAbs = 312u;
constexpr std::uint32_t kLcJumpXyAbs = 321u;

}  // namespace laserdesk::rtc::rif
