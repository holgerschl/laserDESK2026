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
        double rot_deg = 0;
        if (ent.contains("rotation_deg") && ent["rotation_deg"].is_number()) rot_deg = ent["rotation_deg"].get<double>();
        const double pi = std::acos(-1.0);
        const double rad = rot_deg * pi / 180.0;
        const double cx = x + w * 0.5;
        const double cy = y + h * 0.5;
        const double c = std::cos(rad);
        const double s = std::sin(rad);
        auto rot = [&](double px, double py, double& ox, double& oy) {
          const double dx = px - cx;
          const double dy = py - cy;
          ox = cx + dx * c - dy * s;
          oy = cy + dx * s + dy * c;
        };
        double ax0, ay0, ax1, ay1, ax2, ay2, ax3, ay3;
        rot(x, y, ax0, ay0);
        rot(x + w, y, ax1, ay1);
        rot(x + w, y + h, ax2, ay2);
        rot(x, y + h, ax3, ay3);
        auto push_seg = [&](double p0x, double p0y, double p1x, double p1y) {
          dxf::LineEntity le;
          le.index = next_index++;
          le.layer = layer_name;
          le.x0 = p0x;
          le.y0 = p0y;
          le.z0 = z;
          le.x1 = p1x;
          le.y1 = p1y;
          le.z1 = z;
          out.lines.push_back(std::move(le));
        };
        push_seg(ax0, ay0, ax1, ay1);
        push_seg(ax1, ay1, ax2, ay2);
        push_seg(ax2, ay2, ax3, ay3);
        push_seg(ax3, ay3, ax0, ay0);
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
