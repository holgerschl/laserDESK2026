#include "rtc/job/dxf_rif_list_mapper.hpp"
#include "rtc/job/rtc_job_plan.hpp"
#include "rtc/rif/remote_list_commands.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using laserdesk::rtc::job::DxfRifListMapParams;
using laserdesk::rtc::job::RtcJobPlan;
using laserdesk::rtc::job::RtcLineSegment;
using laserdesk::rtc::job::build_dxf_rif_list_upload_sequence;
using laserdesk::rtc::job::parse_rtc_job_plan_from_dxf_json;
using laserdesk::rtc::rif::kLcEndOfList;
using laserdesk::rtc::rif::kLcJumpXyAbs;
using laserdesk::rtc::rif::kLcMarkXyztAbs;

TEST(DxfRifListMapper, ParsePlanFromJson) {
  const nlohmann::json job = nlohmann::json::parse(R"json({
    "kind": "dxf_lines",
    "source_name": "t.dxf",
    "line_count": 1,
    "entities": [
      {"index": 0, "type": "line", "layer": "0",
       "x0": 0, "y0": 0, "z0": 0, "x1": 1, "y1": 2, "z1": 0, "length": 2.236}
    ]
  })json");

  RtcJobPlan plan;
  std::string err;
  ASSERT_TRUE(parse_rtc_job_plan_from_dxf_json(job, plan, err)) << err;
  EXPECT_EQ(plan.source_name, "t.dxf");
  ASSERT_EQ(plan.lines.size(), 1u);
  EXPECT_DOUBLE_EQ(plan.lines[0].x1, 1.0);
  EXPECT_EQ(plan.lines[0].layer, "0");
}

TEST(DxfRifListMapper, OneLineJumpMarkEndOfList) {
  RtcJobPlan plan;
  plan.lines.push_back(RtcLineSegment{0, 0, 0, 10, 20, 0, 0, "0"});

  DxfRifListMapParams p;
  p.bits_per_mm = 128.0;
  p.append_end_of_list = true;

  std::vector<std::vector<std::uint32_t>> seq;
  std::string err;
  ASSERT_TRUE(build_dxf_rif_list_upload_sequence(plan, p, seq, err)) << err;

  ASSERT_EQ(seq.size(), 3u);
  ASSERT_EQ(seq[0].size(), 3u);
  EXPECT_EQ(seq[0][0], kLcJumpXyAbs);
  ASSERT_EQ(seq[1].size(), 6u);
  EXPECT_EQ(seq[1][0], kLcMarkXyztAbs);
  ASSERT_EQ(seq[2].size(), 1u);
  EXPECT_EQ(seq[2][0], kLcEndOfList);
}

TEST(DxfRifListMapper, CoordinatesScaledInJump) {
  RtcJobPlan plan;
  plan.lines.push_back(RtcLineSegment{1.0, -2.0, 0, 0, 0, 0, 0, "0"});
  DxfRifListMapParams p;
  p.bits_per_mm = 10.0;
  p.append_end_of_list = false;

  std::vector<std::vector<std::uint32_t>> seq;
  std::string err;
  ASSERT_TRUE(build_dxf_rif_list_upload_sequence(plan, p, seq, err)) << err;
  ASSERT_GE(seq.size(), 1u);
  const auto& jump = seq[0];
  ASSERT_EQ(jump.size(), 3u);
  const std::int32_t x0 = static_cast<std::int32_t>(jump[1]);
  const std::int32_t y0 = static_cast<std::int32_t>(jump[2]);
  EXPECT_EQ(x0, 10);
  EXPECT_EQ(y0, -20);
}
