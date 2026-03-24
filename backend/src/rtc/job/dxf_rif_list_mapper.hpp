#pragma once

#include "rtc_job_plan.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace laserdesk::rtc::job {

struct DxfRifListMapParams {
  /// DXF user units → scanner coordinates (same scaling idea as RTC DLL / vendor examples).
  double bits_per_mm{128.0};
  /// When **both** set: prepend `R_LC_SET_JUMP_SPEED` / `R_LC_SET_MARK_SPEED` (payload **bits/ms**) before
  /// geometry so streamed lists execute with non-default speeds (ctrl telegrams alone may not apply).
  std::optional<double> list_jump_speed_bits_per_ms;
  std::optional<double> list_mark_speed_bits_per_ms;
  /// Append `R_LC_END_OF_LIST` (259) after geometry (recommended before `R_DC_EXECUTE_LIST_POS`).
  bool append_end_of_list{true};
};

/// Builds one UDP RAW payload per list command: `R_LC_JUMP_XY_ABS` → start of segment,
/// `R_LC_MARK_XYZT_ABS` → mark to end (wrapper: `jump_xy_abs` / `mark_xyzt_abs` in
/// vendor Remote Interface wrapper). IDs must stay in sync with package `telegrams.h`.
bool build_dxf_rif_list_upload_sequence(const RtcJobPlan& plan, const DxfRifListMapParams& params,
                                        std::vector<std::vector<std::uint32_t>>& out, std::string& error);

}  // namespace laserdesk::rtc::job
