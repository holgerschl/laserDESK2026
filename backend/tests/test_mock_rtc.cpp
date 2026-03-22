#include "rtc/mock_rtc_client.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <variant>

using laserdesk::rtc::MockRtcClient;
using laserdesk::rtc::RtcConnectConfig;

TEST(MockRtc, ConnectThenStatus) {
  MockRtcClient rtc;
  RtcConnectConfig cfg;
  cfg.mode = RtcConnectConfig::Mode::Mock;
  ASSERT_FALSE(rtc.connect(cfg).has_value());
  auto st = rtc.get_status();
  ASSERT_TRUE(std::holds_alternative<laserdesk::rtc::RtcStatus>(st));
  const auto& s = std::get<laserdesk::rtc::RtcStatus>(st);
  EXPECT_EQ(s.connection_state, "connected_idle");
  EXPECT_EQ(s.rtc_mode, "mock");
  ASSERT_TRUE(s.package_version_reported.has_value());
  EXPECT_EQ(*s.package_version_reported, "1.22.0-mock");
  EXPECT_TRUE(s.alignment_ok);
}

TEST(MockRtc, DisconnectedGetStatusIsError) {
  MockRtcClient rtc;
  auto st = rtc.get_status();
  ASSERT_TRUE(std::holds_alternative<laserdesk::rtc::RtcError>(st));
  EXPECT_EQ(std::get<laserdesk::rtc::RtcError>(st).code, "RTC_NOT_CONNECTED");
}

TEST(MockRtc, LoadStartStopFlow) {
  MockRtcClient rtc;
  RtcConnectConfig cfg{RtcConnectConfig::Mode::Mock};
  ASSERT_FALSE(rtc.connect(cfg).has_value());
  auto job = rtc.load_minimal_job("test-label");
  ASSERT_TRUE(std::holds_alternative<std::string>(job));
  EXPECT_FALSE(std::get<std::string>(job).empty());

  auto st1 = std::get<laserdesk::rtc::RtcStatus>(rtc.get_status());
  EXPECT_EQ(st1.connection_state, "loaded");

  ASSERT_FALSE(rtc.start_execution().has_value());
  EXPECT_EQ(std::get<laserdesk::rtc::RtcStatus>(rtc.get_status()).connection_state, "running");

  ASSERT_FALSE(rtc.stop_execution().has_value());
  EXPECT_EQ(std::get<laserdesk::rtc::RtcStatus>(rtc.get_status()).connection_state, "loaded");
}

TEST(MockRtc, StartWithoutLoadFails) {
  MockRtcClient rtc;
  ASSERT_FALSE(rtc.connect(RtcConnectConfig{RtcConnectConfig::Mode::Mock}).has_value());
  auto e = rtc.start_execution();
  ASSERT_TRUE(e.has_value());
  EXPECT_EQ(e->code, "RTC_NOT_READY");
}

TEST(MockRtc, StopWithoutRunFails) {
  MockRtcClient rtc;
  ASSERT_FALSE(rtc.connect(RtcConnectConfig{RtcConnectConfig::Mode::Mock}).has_value());
  auto e = rtc.stop_execution();
  ASSERT_TRUE(e.has_value());
  EXPECT_EQ(e->code, "RTC_NOT_RUNNING");
}

TEST(MockRtc, DisconnectResets) {
  MockRtcClient rtc;
  ASSERT_FALSE(rtc.connect(RtcConnectConfig{RtcConnectConfig::Mode::Mock}).has_value());
  ASSERT_TRUE(std::holds_alternative<std::string>(rtc.load_minimal_job("")));
  rtc.disconnect();
  auto st = rtc.get_status();
  ASSERT_TRUE(std::holds_alternative<laserdesk::rtc::RtcError>(st));
}

TEST(MockRtc, LoadDxfJobStartStop) {
  MockRtcClient rtc;
  ASSERT_FALSE(rtc.connect(RtcConnectConfig{RtcConnectConfig::Mode::Mock}).has_value());
  nlohmann::json job{{"source_name", "t.dxf"},
                      {"line_count", 2},
                      {"entities", nlohmann::json::array({{{"type", "line"}}, {{"type", "line"}}})}};
  ASSERT_FALSE(rtc.load_dxf_job(job).has_value());
  auto st = std::get<laserdesk::rtc::RtcStatus>(rtc.get_status());
  EXPECT_EQ(st.connection_state, "loaded");
  ASSERT_TRUE(st.active_dxf_line_count.has_value());
  EXPECT_EQ(*st.active_dxf_line_count, 2u);
  ASSERT_FALSE(rtc.start_execution().has_value());
  ASSERT_FALSE(rtc.stop_execution().has_value());
}
