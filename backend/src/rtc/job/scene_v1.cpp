#include "scene_v1.hpp"

#include <cmath>
#include <sstream>

namespace laserdesk::rtc::job {
namespace {

bool need_num(const nlohmann::json& o, const char* key, std::string& err) {
  if (!o.contains(key) || !o[key].is_number()) {
    err = std::string("missing or non-numeric '") + key + "'";
    return false;
  }
  return true;
}

}  // namespace

bool scene_v1_to_parse_result(const nlohmann::json& scene, dxf::ParseResult& out, std::string& error) {
  out = dxf::ParseResult{};
  if (!scene.is_object()) {
    error = "scene root must be an object";
    return false;
  }
  if (!scene.contains("schemaVersion") || !scene["schemaVersion"].is_number_integer() ||
      scene["schemaVersion"].get<int>() != 1) {
    error = "schemaVersion must be 1";
    return false;
  }
  if (!scene.contains("kind") || !scene["kind"].is_string() || scene["kind"].get<std::string>() != "scene_v1") {
    error = "kind must be \"scene_v1\"";
    return false;
  }
  std::string source = "scene";
  if (scene.contains("source_name") && scene["source_name"].is_string())
    source = scene["source_name"].get<std::string>();
  out.source_name = source;

  if (!scene.contains("layers") || !scene["layers"].is_array() || scene["layers"].empty()) {
    error = "layers must be a non-empty array";
    return false;
  }

  int next_index = 0;
  for (const auto& layer : scene["layers"]) {
    if (!layer.is_object()) {
      error = "layer is not an object";
      return false;
    }
    std::string layer_name = "0";
    if (layer.contains("name") && layer["name"].is_string()) layer_name = layer["name"].get<std::string>();
    else if (layer.contains("id") && layer["id"].is_string()) layer_name = layer["id"].get<std::string>();

    if (!layer.contains("entities") || !layer["entities"].is_array()) {
      error = "layer.entities must be an array";
      return false;
    }

    for (const auto& ent : layer["entities"]) {
      if (!ent.is_object()) {
        error = "entity is not an object";
        return false;
      }
      if (!ent.contains("type") || !ent["type"].is_string()) {
        error = "entity.type missing";
        return false;
      }
      const std::string t = ent["type"].get<std::string>();
      if (t == "line") {
        for (const char* k : {"x0", "y0", "z0", "x1", "y1", "z1"}) {
          if (!need_num(ent, k, error)) return false;
        }
        dxf::LineEntity le;
        le.index = next_index++;
        le.layer = layer_name;
        le.x0 = ent["x0"].get<double>();
        le.y0 = ent["y0"].get<double>();
        le.z0 = ent["z0"].get<double>();
        le.x1 = ent["x1"].get<double>();
        le.y1 = ent["y1"].get<double>();
        le.z1 = ent["z1"].get<double>();
        out.lines.push_back(std::move(le));
      } else if (t == "rect") {
        for (const char* k : {"x", "y", "width", "height"}) {
          if (!need_num(ent, k, error)) return false;
        }
        double z = 0;
        if (ent.contains("z") && ent["z"].is_number()) z = ent["z"].get<double>();
        const double x = ent["x"].get<double>();
        const double y = ent["y"].get<double>();
        const double w = ent["width"].get<double>();
        const double h = ent["height"].get<double>();
        if (w <= 0 || h <= 0) {
          error = "rect width and height must be positive";
          return false;
        }
        const double x0 = x, y0 = y, x1 = x + w, y1 = y, x2 = x + w, y2 = y + h, x3 = x, y3 = y + h;
        auto push_seg = [&](double ax, double ay, double bx, double by) {
          dxf::LineEntity le;
          le.index = next_index++;
          le.layer = layer_name;
          le.x0 = ax;
          le.y0 = ay;
          le.z0 = z;
          le.x1 = bx;
          le.y1 = by;
          le.z1 = z;
          out.lines.push_back(std::move(le));
        };
        push_seg(x0, y0, x1, y1);
        push_seg(x1, y1, x2, y2);
        push_seg(x2, y2, x3, y3);
        push_seg(x3, y3, x0, y0);
      } else {
        std::ostringstream os;
        os << "unsupported entity type: " << t;
        error = os.str();
        return false;
      }
    }
  }

  if (out.lines.empty()) {
    error = "no line segments produced (empty scene or invalid entities)";
    return false;
  }
  return true;
}

}  // namespace laserdesk::rtc::job
