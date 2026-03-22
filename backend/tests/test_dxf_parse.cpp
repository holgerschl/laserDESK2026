#include "dxf/ascii_dxf_lines.hpp"

#include <fstream>
#include <gtest/gtest.h>
#include <string>

namespace {

std::string read_fixture() {
  std::ifstream in(LASERDESK_TEST_DXF_FIXTURE, std::ios::binary);
  EXPECT_TRUE(in) << "open " << LASERDESK_TEST_DXF_FIXTURE;
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

}  // namespace

TEST(DxfParse, ScanlabLogoFirstLine) {
  const std::string raw = read_fixture();
  ASSERT_FALSE(raw.empty());
  auto pr = laserdesk::dxf::parse_ascii_dxf_lines("SCANLABLogo.dxf", raw);
  ASSERT_FALSE(pr.error_code.has_value()) << pr.error_message.value_or("");
  ASSERT_GE(pr.lines.size(), 10u);
  const auto& e0 = pr.lines[0];
  EXPECT_EQ(e0.index, 0);
  EXPECT_DOUBLE_EQ(e0.x0, 0.0);
  EXPECT_DOUBLE_EQ(e0.y0, 10.0);
  EXPECT_DOUBLE_EQ(e0.z0, 0.0);
  EXPECT_DOUBLE_EQ(e0.x1, -10.0);
  EXPECT_DOUBLE_EQ(e0.y1, 0.0);
  EXPECT_DOUBLE_EQ(e0.z1, 0.0);
}

TEST(DxfParse, JobJsonRoundTripShape) {
  const std::string raw = read_fixture();
  auto pr = laserdesk::dxf::parse_ascii_dxf_lines("x.dxf", raw);
  ASSERT_FALSE(pr.error_code.has_value());
  auto j = laserdesk::dxf::job_to_json("job-1", pr);
  EXPECT_EQ(j["job_id"], "job-1");
  EXPECT_EQ(j["kind"], "dxf_lines");
  EXPECT_TRUE(j["entities"].is_array());
  EXPECT_EQ(j["line_count"], j["entities"].size());
}
