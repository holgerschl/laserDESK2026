#include "http/api_router.hpp"

#include <gtest/gtest.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

using laserdesk::http_api::BackendSession;

TEST(BackendSession, ConnectMockAndHealth) {
  BackendSession s;
  nlohmann::json err;
  EXPECT_EQ(s.handle_post_rtc_connect(nlohmann::json{{"mode", "mock"}}, err), 204);

  auto h = s.handle_get_health();
  EXPECT_EQ(h["status"].get<std::string>(), "ok");
  EXPECT_EQ(h["rtc_mode"].get<std::string>(), "mock");
}

TEST(BackendSession, EthernetConnectMissingHost400) {
  BackendSession s;
  nlohmann::json err;
  EXPECT_EQ(s.handle_post_rtc_connect(nlohmann::json{{"mode", "ethernet"}}, err), 400);
  EXPECT_EQ(err["code"].get<std::string>(), "RTC_INTERNAL");
}

TEST(BackendSession, JobFlowAfterConnect) {
  BackendSession s;
  nlohmann::json err;
  ASSERT_EQ(s.handle_post_rtc_connect(nlohmann::json{{"mode", "mock"}}, err), 204);

  nlohmann::json out;
  ASSERT_EQ(s.handle_post_minimal_demo_job(nlohmann::json{{"label", "e2e"}}, out, err), 200);
  ASSERT_TRUE(out.contains("job_id"));
  EXPECT_FALSE(out["job_id"].get<std::string>().empty());

  httplib::Request run_req;
  EXPECT_EQ(s.handle_post_minimal_demo_run(run_req, err), 204);
  EXPECT_EQ(s.handle_post_minimal_demo_stop(err), 204);
}

TEST(BackendSession, JobRunInvalidRepeatCount400) {
  BackendSession s;
  nlohmann::json err;
  ASSERT_EQ(s.handle_post_rtc_connect(nlohmann::json{{"mode", "mock"}}, err), 204);
  httplib::Request run_req;
  run_req.params.emplace("repeat_count", "0");
  EXPECT_EQ(s.handle_post_minimal_demo_run(run_req, err), 400);
}

TEST(BackendSession, JobWithoutConnect409) {
  BackendSession s;
  nlohmann::json err, out;
  EXPECT_EQ(s.handle_post_minimal_demo_job({}, out, err), 409);
  EXPECT_EQ(err["code"].get<std::string>(), "RTC_NOT_CONNECTED");
}

TEST(BackendSession, RtcStopIdempotentWhenNotRunning) {
  BackendSession s;
  nlohmann::json err;
  ASSERT_EQ(s.handle_post_rtc_connect(nlohmann::json{{"mode", "mock"}}, err), 204);
  EXPECT_EQ(s.handle_post_rtc_stop(err), 204);
}

TEST(BackendSession, RifLogShape) {
  BackendSession s;
  nlohmann::json j = s.handle_get_rtc_rif_log();
  ASSERT_TRUE(j.contains("lines"));
  EXPECT_TRUE(j["lines"].is_array());
  EXPECT_TRUE(j.contains("hint"));

  nlohmann::json err;
  ASSERT_EQ(s.handle_post_rtc_connect(nlohmann::json{{"mode", "mock"}}, err), 204);
  nlohmann::json j2 = s.handle_get_rtc_rif_log();
  EXPECT_TRUE(j2.contains("lines"));
  EXPECT_TRUE(j2.contains("rtc_mode"));
  EXPECT_EQ(j2["rtc_mode"].get<std::string>(), "mock");
}
