#include "ascii_dxf_lines.hpp"

#include <cmath>
#include <cstdlib>

namespace laserdesk::dxf {
namespace {

std::string trim(std::string s) {
  while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(s.begin());
  while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
  return s;
}

std::vector<std::string> split_lines(const std::string& text) {
  std::vector<std::string> out;
  std::size_t start = 0;
  while (start <= text.size()) {
    std::size_t end = text.find('\n', start);
    std::string line =
        end == std::string::npos ? text.substr(start) : text.substr(start, end - start);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    out.push_back(std::move(line));
    if (end == std::string::npos) break;
    start = end + 1;
  }
  return out;
}

bool parse_int(const std::string& s, int& out) {
  try {
    std::size_t pos = 0;
    long v = std::stol(s, &pos, 10);
    if (pos != s.size() || v < -1000000 || v > 1000000) return false;
    out = static_cast<int>(v);
    return true;
  } catch (...) {
    return false;
  }
}

bool parse_double(const std::string& s, double& out) {
  try {
    std::size_t pos = 0;
    out = std::stod(s, &pos);
    return pos == s.size();
  } catch (...) {
    return false;
  }
}

ParseResult fail(std::string source, std::string code, std::string msg) {
  ParseResult r;
  r.source_name = std::move(source);
  r.error_code = std::move(code);
  r.error_message = std::move(msg);
  return r;
}

bool read_pair(const std::vector<std::string>& lines, std::size_t& i, int& code, std::string& value) {
  if (i + 1 >= lines.size()) return false;
  if (!parse_int(trim(lines[i]), code)) return false;
  value = trim(lines[i + 1]);
  i += 2;
  return true;
}

/// After (0, entity_name), skip attribute pairs until next group 0 (start of next entity) or EOF.
void skip_entity(const std::vector<std::string>& lines, std::size_t& i) {
  while (i + 1 < lines.size()) {
    int c = 0;
    if (!parse_int(trim(lines[i]), c)) {
      i += 2;
      continue;
    }
    if (c == 0) return;
    i += 2;
  }
}

/// Parse LINE attributes; `i` is first pair after (0,LINE). Leaves `i` at the next (0,…) pair.
bool parse_line_entity(const std::vector<std::string>& lines, std::size_t& i, LineEntity& ent) {
  std::string layer = "0";
  double x0 = 0, y0 = 0, z0 = 0, x1 = 0, y1 = 0, z1 = 0;
  bool has10 = false, has20 = false, has11 = false, has21 = false;

  while (i + 1 < lines.size()) {
    int code = 0;
    if (!parse_int(trim(lines[i]), code)) return false;
    std::string val = trim(lines[i + 1]);
    if (code == 0) {
      if (!has10 || !has20 || !has11 || !has21) return false;
      ent.layer = std::move(layer);
      ent.x0 = x0;
      ent.y0 = y0;
      ent.z0 = z0;
      ent.x1 = x1;
      ent.y1 = y1;
      ent.z1 = z1;
      return true;
    }
    i += 2;
    switch (code) {
      case 8:
        layer = val;
        break;
      case 10:
        if (!parse_double(val, x0)) return false;
        has10 = true;
        break;
      case 20:
        if (!parse_double(val, y0)) return false;
        has20 = true;
        break;
      case 30:
        if (!parse_double(val, z0)) return false;
        break;
      case 11:
        if (!parse_double(val, x1)) return false;
        has11 = true;
        break;
      case 21:
        if (!parse_double(val, y1)) return false;
        has21 = true;
        break;
      case 31:
        if (!parse_double(val, z1)) return false;
        break;
      default:
        break;
    }
  }
  return false;
}

}  // namespace

ParseResult parse_ascii_dxf_lines(std::string source_name, const std::string& text) {
  if (text.empty()) return fail(std::move(source_name), "DXF_PARSE_ERROR", "empty DXF content");

  const std::vector<std::string> lines = split_lines(text);
  ParseResult out;
  out.source_name = std::move(source_name);

  std::size_t i = 0;
  bool in_entities = false;

  while (i + 1 < lines.size()) {
    int code = 0;
    std::string val;
    if (!read_pair(lines, i, code, val)) break;

    if (code == 0 && val == "SECTION") {
      int c2 = 0;
      std::string v2;
      if (!read_pair(lines, i, c2, v2) || c2 != 2) {
        return fail(out.source_name, "DXF_PARSE_ERROR", "malformed SECTION header");
      }
      in_entities = (v2 == "ENTITIES");
      continue;
    }

    if (!in_entities) continue;

    if (code == 0 && val == "ENDSEC") {
      in_entities = false;
      continue;
    }

    if (code == 0 && val == "LINE") {
      LineEntity ent;
      if (!parse_line_entity(lines, i, ent)) {
        return fail(out.source_name, "DXF_PARSE_ERROR", "invalid LINE entity");
      }
      ent.index = static_cast<int>(out.lines.size());
      out.lines.push_back(ent);
      continue;
    }

    if (code == 0) {
      skip_entity(lines, i);
      continue;
    }
  }

  if (out.lines.empty()) {
    return fail(std::move(out.source_name), "DXF_PARSE_ERROR", "no LINE entities found in ENTITIES section");
  }

  return out;
}

nlohmann::json job_to_json(const std::string& job_id, const ParseResult& pr) {
  nlohmann::json j;
  j["job_id"] = job_id;
  j["source_name"] = pr.source_name;
  j["kind"] = "dxf_lines";
  j["line_count"] = pr.lines.size();
  nlohmann::json ents = nlohmann::json::array();
  for (const auto& e : pr.lines) {
    const double dx = e.x1 - e.x0;
    const double dy = e.y1 - e.y0;
    const double dz = e.z1 - e.z0;
    const double len = std::sqrt(dx * dx + dy * dy + dz * dz);
    ents.push_back(nlohmann::json{{"index", e.index},
                                  {"type", "line"},
                                  {"layer", e.layer},
                                  {"x0", e.x0},
                                  {"y0", e.y0},
                                  {"z0", e.z0},
                                  {"x1", e.x1},
                                  {"y1", e.y1},
                                  {"z1", e.z1},
                                  {"length", len}});
  }
  j["entities"] = std::move(ents);
  if (pr.job_jump_speed_mm_s.has_value() && pr.job_mark_speed_mm_s.has_value()) {
    j["laser"] = nlohmann::json{{"jump_speed_mm_s", *pr.job_jump_speed_mm_s},
                                {"mark_speed_mm_s", *pr.job_mark_speed_mm_s}};
  }
  return j;
}

}  // namespace laserdesk::dxf
