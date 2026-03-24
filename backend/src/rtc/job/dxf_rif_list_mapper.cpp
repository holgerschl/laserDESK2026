#include "dxf_rif_list_mapper.hpp"

#include "rtc/rif/remote_list_commands.hpp"

#include <cmath>
#include <cstring>

namespace laserdesk::rtc::job {

namespace {

bool to_scan_bits(double mm, double bits_per_mm, std::int32_t& out, std::string& error) {
  const long long q = llround(mm * bits_per_mm);
  if (q < -524288LL || q > 524287LL) {
    error = "DXF coordinate out of 20-bit signed range after bits_per_mm scaling";
    return false;
  }
  out = static_cast<std::int32_t>(q);
  return true;
}

void append_i32_as_u32(std::vector<std::uint32_t>& w, std::int32_t v) {
  w.push_back(static_cast<std::uint32_t>(v));
}

void append_double_le(std::vector<std::uint32_t>& w, double d) {
  std::uint32_t pair[2];
  static_assert(sizeof(d) == sizeof(pair), "double size");
  std::memcpy(pair, &d, sizeof(d));
  w.push_back(pair[0]);
  w.push_back(pair[1]);
}

}  // namespace

bool build_dxf_rif_list_upload_sequence(const RtcJobPlan& plan, const DxfRifListMapParams& params,
                                        std::vector<std::vector<std::uint32_t>>& out, std::string& error) {
  out.clear();
  if (plan.lines.empty()) {
    error = "RtcJobPlan has no lines";
    return false;
  }
  if (params.bits_per_mm <= 0.0) {
    error = "bits_per_mm must be positive";
    return false;
  }

  // Some firmware treats 0 as a degenerate mark; use a small positive dwell (seconds, SCANLAB list semantics).
  constexpr double kMarkTime = 1.0e-4;

  for (const auto& ln : plan.lines) {
    std::int32_t x0, y0, x1, y1, z1;
    if (!to_scan_bits(ln.x0, params.bits_per_mm, x0, error)) return false;
    if (!to_scan_bits(ln.y0, params.bits_per_mm, y0, error)) return false;
    if (!to_scan_bits(ln.x1, params.bits_per_mm, x1, error)) return false;
    if (!to_scan_bits(ln.y1, params.bits_per_mm, y1, error)) return false;
    if (!to_scan_bits(ln.z1, params.bits_per_mm, z1, error)) return false;

    {
      std::vector<std::uint32_t> jump;
      jump.reserve(3);
      jump.push_back(rif::kLcJumpXyAbs);
      append_i32_as_u32(jump, x0);
      append_i32_as_u32(jump, y0);
      out.push_back(std::move(jump));
    }
    {
      std::vector<std::uint32_t> mark;
      mark.reserve(6);
      mark.push_back(rif::kLcMarkXyztAbs);
      append_i32_as_u32(mark, x1);
      append_i32_as_u32(mark, y1);
      append_i32_as_u32(mark, z1);
      append_double_le(mark, kMarkTime);
      out.push_back(std::move(mark));
    }
  }

  if (params.append_end_of_list) {
    out.push_back({rif::kLcEndOfList});
  }
  return true;
}

}  // namespace laserdesk::rtc::job
