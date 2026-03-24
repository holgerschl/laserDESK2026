#include "http/api_router.hpp"

#include <gtest/gtest.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

namespace {

std::string read_fixture() {
  std::ifstream in(LASERDESK_TEST_DXF_FIXTURE, std::ios::binary);
  EXPECT_TRUE(in);
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

}  // namespace

TEST(BackendSessionDxf, ParseLoadRunStopFlow) {
  laserdesk::http_api::BackendSession s;
  nlohmann::json err;

  ASSERT_EQ(s.handle_post_rtc_connect(nlohmann::json{{"mode", "mock"}}, err), 204);

  httplib::Request req;
  req.body = nlohmann::json{{"dxf_text", read_fixture()}, {"source_name", "SCANLABLogo.dxf"}}.dump();

  nlohmann::json out;
  ASSERT_EQ(s.handle_post_jobs_dxf(req, out, err), 200) << err.dump();
  ASSERT_TRUE(out.contains("job_id"));
  const std::string job_id = out["job_id"].get<std::string>();

  nlohmann::json job_body;
  ASSERT_EQ(s.handle_get_jobs_dxf(job_id, job_body, err), 200);
  EXPECT_TRUE(job_body.contains("entities"));
  EXPECT_GE(job_body["line_count"].get<std::size_t>(), 10u);

  EXPECT_EQ(s.handle_post_jobs_dxf_load(job_id, err), 204) << err.dump();
  httplib::Request run_req;
  EXPECT_EQ(s.handle_post_jobs_dxf_run(run_req, err), 204) << err.dump();
  EXPECT_EQ(s.handle_post_jobs_dxf_stop(err), 204) << err.dump();
}
