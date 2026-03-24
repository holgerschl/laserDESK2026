#pragma once

#include <cstdint>

namespace laserdesk::rtc::rif {

/// RTC6 Manual Doc. Rev. 1.1.3 en-US, Ch. 10 Ctrl Command `get_status` — `Status` word bit meanings.
/// @see `docs/RTC6_Doc.Rev_.1.1.3_en-US.pdf` (PDF page ~497, `get_status` returned parameter `Status`).
constexpr std::uint32_t kRdcGetStatusBusyListExecution = 1u << 0u;   ///< BUSY list execution status
constexpr std::uint32_t kRdcGetStatusInternalBusy = 1u << 7u;        ///< INTERNAL-BUSY list execution status
constexpr std::uint32_t kRdcGetStatusPaused = 1u << 15u;           ///< PAUSED list execution status
constexpr std::uint32_t kRdcGetStatusHeadBusy = 1u << 23u;         ///< HEAD BUSY (SCANahead)

/// When **all** of these bits are clear, list execution is not active (Ch. 6.4.3 List Execution Status).
constexpr std::uint32_t kRdcGetStatusListExecutionBusyMask =
    kRdcGetStatusBusyListExecution | kRdcGetStatusInternalBusy | kRdcGetStatusPaused | kRdcGetStatusHeadBusy;

}  // namespace laserdesk::rtc::rif
