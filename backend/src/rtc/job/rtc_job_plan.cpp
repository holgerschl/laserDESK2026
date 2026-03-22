#include "rtc_job_plan.hpp"

#include <nlohmann/json.hpp>

namespace laserdesk::rtc::job {

namespace {

bool need_number(const nlohmann::json& j, const char* key, std::string& error) {
  if (!j.contains(key) || !j[key].is_number()) {
    error = std::string("entity missing numeric ") + key;
    return false;
  }
  return true;
}

}  // namespace

bool parse_rtc_job_plan_from_dxf_json(const nlohmann::json& job, RtcJobPlan& out, std::string& error) {
  out = RtcJobPlan{};
  if (!job.contains("kind") || !job["kind"].is_string() || job["kind"].get<std::string>() != "dxf_lines") {
    error = "job kind is not dxf_lines";
    return false;
  }
  if (!job.contains("entities") || !job["entities"].is_array() || job["entities"].empty()) {
    error = "job has no entities";
    return false;
  }
  if (job.contains("source_name") && job["source_name"].is_string()) {
    out.source_name = job["source_name"].get<std::string>();
  }
  for (const auto& ent : job["entities"]) {
    if (!ent.is_object()) {
      error = "entity is not an object";
      return false;
    }
    if (!ent.contains("type") || !ent["type"].is_string() || ent["type"].get<std::string>() != "line") {
      error = "only type \"line\" is supported in RtcJobPlan";
      return false;
    }
    for (const char* k : {"x0", "y0", "z0", "x1", "y1", "z1"}) {
      if (!need_number(ent, k, error)) return false;
    }
    if (!ent.contains("index") || !ent["index"].is_number_integer()) {
      error = "entity missing integer index";
      return false;
    }
    RtcLineSegment s;
    s.x0 = ent["x0"].get<double>();
    s.y0 = ent["y0"].get<double>();
    s.z0 = ent["z0"].get<double>();
    s.x1 = ent["x1"].get<double>();
    s.y1 = ent["y1"].get<double>();
    s.z1 = ent["z1"].get<double>();
    s.index = ent["index"].get<int>();
    if (ent.contains("layer") && ent["layer"].is_string()) s.layer = ent["layer"].get<std::string>();
    out.lines.push_back(std::move(s));
  }
  return true;
}

}  // namespace laserdesk::rtc::job
