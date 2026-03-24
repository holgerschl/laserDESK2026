#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace laserdesk::dxf {

struct LineEntity {
  int index{0};
  std::string layer;
  double x0{0};
  double y0{0};
  double z0{0};
  double x1{0};
  double y1{0};
  double z1{0};
};

struct ParseResult {
  std::string source_name;
  std::vector<LineEntity> lines;
  /// From `scene_v1` (`laser_groups` / entity `laser`) for `POST /jobs/scene` → GET job JSON → Ethernet execute.
  std::optional<double> job_jump_speed_mm_s;
  std::optional<double> job_mark_speed_mm_s;
  /// Set on failure
  std::optional<std::string> error_code;
  std::optional<std::string> error_message;
};

/// Parse ENTITIES section: LINE entities only (ASCII DXF). Other entity types are skipped.
ParseResult parse_ascii_dxf_lines(std::string source_name, const std::string& text);

/// Serialise job for API / RTC (stable shape for UI and load_dxf_job).
nlohmann::json job_to_json(const std::string& job_id, const ParseResult& pr);

}  // namespace laserdesk::dxf
