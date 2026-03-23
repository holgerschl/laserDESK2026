#pragma once

#include "dxf/ascii_dxf_lines.hpp"

#include <nlohmann/json.hpp>
#include <string>

namespace laserdesk::rtc::job {

/// Validates `scene_v1` JSON and fills `ParseResult` with LINE entities (rect → 4 segments).
bool scene_v1_to_parse_result(const nlohmann::json& scene, dxf::ParseResult& out, std::string& error);

}  // namespace laserdesk::rtc::job
