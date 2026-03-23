#include <gtest/gtest.h>

#include "rtc/job/scene_v1.hpp"

#include <nlohmann/json.hpp>

TEST(SceneV1, Line) {
  auto j = nlohmann::json::parse(R"({
    "schemaVersion": 1,
    "kind": "scene_v1",
    "source_name": "t",
    "layers": [
      {
        "name": "L1",
        "entities": [
          { "type": "line", "x0": 0, "y0": 0, "z0": 0, "x1": 10, "y1": 0, "z1": 0 }
        ]
      }
    ]
  })");
  laserdesk::dxf::ParseResult pr;
  std::string err;
  ASSERT_TRUE(laserdesk::rtc::job::scene_v1_to_parse_result(j, pr, err)) << err;
  ASSERT_EQ(pr.lines.size(), 1u);
  EXPECT_EQ(pr.lines[0].layer, "L1");
  EXPECT_DOUBLE_EQ(pr.lines[0].x1, 10.0);
}

TEST(SceneV1, RectFourLines) {
  auto j = nlohmann::json::parse(R"({
    "schemaVersion": 1,
    "kind": "scene_v1",
    "layers": [
      {
        "id": "0",
        "entities": [
          { "type": "rect", "x": 0, "y": 0, "width": 10, "height": 5, "z": 0 }
        ]
      }
    ]
  })");
  laserdesk::dxf::ParseResult pr;
  std::string err;
  ASSERT_TRUE(laserdesk::rtc::job::scene_v1_to_parse_result(j, pr, err)) << err;
  ASSERT_EQ(pr.lines.size(), 4u);
}

TEST(SceneV1, RectRotatedProducesFourLines) {
  auto j = nlohmann::json::parse(R"({
    "schemaVersion": 1,
    "kind": "scene_v1",
    "layers": [
      {
        "id": "0",
        "entities": [
          { "type": "rect", "x": 0, "y": 0, "width": 10, "height": 5, "z": 0, "rotation_deg": 45 }
        ]
      }
    ]
  })");
  laserdesk::dxf::ParseResult pr;
  std::string err;
  ASSERT_TRUE(laserdesk::rtc::job::scene_v1_to_parse_result(j, pr, err)) << err;
  ASSERT_EQ(pr.lines.size(), 4u);
}

TEST(SceneV1, RejectsBadVersion) {
  auto j = nlohmann::json::parse(R"({"schemaVersion": 2, "kind": "scene_v1", "layers": [{"entities": []}]})");
  laserdesk::dxf::ParseResult pr;
  std::string err;
  EXPECT_FALSE(laserdesk::rtc::job::scene_v1_to_parse_result(j, pr, err));
}

TEST(SceneV1, ArcProducesSegments) {
  auto j = nlohmann::json::parse(R"({
    "schemaVersion": 1,
    "kind": "scene_v1",
    "layers": [
      {
        "name": "0",
        "entities": [
          {
            "type": "arc",
            "cx": 0,
            "cy": 0,
            "radius": 10,
            "start_angle_deg": 0,
            "sweep_angle_deg": 90
          }
        ]
      }
    ]
  })");
  laserdesk::dxf::ParseResult pr;
  std::string err;
  ASSERT_TRUE(laserdesk::rtc::job::scene_v1_to_parse_result(j, pr, err)) << err;
  EXPECT_GE(pr.lines.size(), 8u);
  EXPECT_LE(pr.lines.size(), 120u);
}

TEST(SceneV1, TextProducesFiveSegments) {
  auto j = nlohmann::json::parse(R"({
    "schemaVersion": 1,
    "kind": "scene_v1",
    "layers": [
      {
        "entities": [
          {
            "type": "text",
            "x": 10,
            "y": 20,
            "height_mm": 5,
            "text": "Hi"
          }
        ]
      }
    ]
  })");
  laserdesk::dxf::ParseResult pr;
  std::string err;
  ASSERT_TRUE(laserdesk::rtc::job::scene_v1_to_parse_result(j, pr, err)) << err;
  EXPECT_EQ(pr.lines.size(), 5u);
}
