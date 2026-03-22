#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace laserdesk::rtc::job {

/// One DXF LINE mapped to user-space coordinates (same units as parser / API).
struct RtcLineSegment {
  double x0{};
  double y0{};
  double z0{};
  double x1{};
  double y1{};
  double z1{};
  int index{};
  std::string layer;
};

/// Normalised plan for RTC list generation (Phase G.4).
struct RtcJobPlan {
  std::string source_name;
  std::vector<RtcLineSegment> lines;
};

/// Fills `out` from GET /jobs/dxf/{id} style JSON (`kind`, `entities`, …).
bool parse_rtc_job_plan_from_dxf_json(const nlohmann::json& job, RtcJobPlan& out, std::string& error);

}  // namespace laserdesk::rtc::job
