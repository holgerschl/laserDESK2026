#include "scene_v1.hpp"

#include <algorithm>
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
      } else if (t == "arc") {
        for (const char* k : {"cx", "cy", "radius", "start_angle_deg", "sweep_angle_deg"}) {
          if (!need_num(ent, k, error)) return false;
        }
        double z = 0;
        if (ent.contains("cz") && ent["cz"].is_number()) z = ent["cz"].get<double>();
        const double cx = ent["cx"].get<double>();
        const double cy = ent["cy"].get<double>();
        const double r = ent["radius"].get<double>();
        const double start_deg = ent["start_angle_deg"].get<double>();
        const double sweep_deg = ent["sweep_angle_deg"].get<double>();
        if (r <= 0) {
          error = "arc radius must be positive";
          return false;
        }
        const double pi = std::acos(-1.0);
        const double abs_sweep = std::abs(sweep_deg);
        int n = std::max(8, std::min(96, static_cast<int>(std::ceil(abs_sweep / 3.0)) + 1));
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
        for (int i = 0; i < n; ++i) {
          const double t0 = static_cast<double>(i) / static_cast<double>(n);
          const double t1 = static_cast<double>(i + 1) / static_cast<double>(n);
          const double a0 = (start_deg + sweep_deg * t0) * pi / 180.0;
          const double a1 = (start_deg + sweep_deg * t1) * pi / 180.0;
          const double x0 = cx + r * std::cos(a0);
          const double y0 = cy + r * std::sin(a0);
          const double x1 = cx + r * std::cos(a1);
          const double y1 = cy + r * std::sin(a1);
          push_seg(x0, y0, x1, y1);
        }
      } else if (t == "text") {
        for (const char* k : {"x", "y", "height_mm"}) {
          if (!need_num(ent, k, error)) return false;
        }
        if (!ent.contains("text") || !ent["text"].is_string()) {
          error = "text entity requires string \"text\"";
          return false;
        }
        double z = 0;
        if (ent.contains("z") && ent["z"].is_number()) z = ent["z"].get<double>();
        const double cx = ent["x"].get<double>();
        const double cy = ent["y"].get<double>();
        const double h = ent["height_mm"].get<double>();
        const std::string text = ent["text"].get<std::string>();
        if (h <= 0) {
          error = "text height_mm must be positive";
          return false;
        }
        double rot_deg = 0;
        if (ent.contains("rotation_deg") && ent["rotation_deg"].is_number()) rot_deg = ent["rotation_deg"].get<double>();
        const double w =
            std::max(1.0, static_cast<double>(std::max<std::size_t>(1, text.size())) * h * 0.52);
        const double pi = std::acos(-1.0);
        const double rad = rot_deg * pi / 180.0;
        const double c = std::cos(rad);
        const double s = std::sin(rad);
        const double hw = w * 0.5;
        const double hh = h * 0.5;
        auto rot = [&](double px, double py, double& ox, double& oy) {
          const double dx = px - cx;
          const double dy = py - cy;
          ox = cx + dx * c - dy * s;
          oy = cy + dx * s + dy * c;
        };
        double ax0, ay0, ax1, ay1, ax2, ay2, ax3, ay3;
        rot(cx - hw, cy - hh, ax0, ay0);
        rot(cx + hw, cy - hh, ax1, ay1);
        rot(cx + hw, cy + hh, ax2, ay2);
        rot(cx - hw, cy + hh, ax3, ay3);
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
        push_seg(ax0, ay0, ax2, ay2);
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
