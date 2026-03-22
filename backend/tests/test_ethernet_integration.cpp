#include "rtc/ethernet_rtc_client.hpp"

#include <cstdlib>
#include <gtest/gtest.h>
#include <variant>

using laserdesk::rtc::EthernetRtcClient;
using laserdesk::rtc::RtcConnectConfig;

/// Set LASERDESK_RTC6_HOST and optional LASERDESK_RTC6_PORT (default 5020) for hardware validation.
TEST(EthernetRtcIntegration, ConnectAndGetStatus) {
  const char* host = std::getenv("LASERDESK_RTC6_HOST");
  if (!host || !*host) {
    GTEST_SKIP() << "Set LASERDESK_RTC6_HOST to run integration test";
  }
  int port = 5020;
  if (const char* ps = std::getenv("LASERDESK_RTC6_PORT")) {
    port = std::atoi(ps);
  }

  EthernetRtcClient rtc;
  RtcConnectConfig cfg;
  cfg.mode = RtcConnectConfig::Mode::Ethernet;
  cfg.host = host;
  cfg.port = port;
  cfg.tgm_format = 0;
  cfg.recv_timeout_ms = 2000;
  ASSERT_FALSE(rtc.connect(cfg).has_value());

  auto st = rtc.get_status();
  ASSERT_TRUE(std::holds_alternative<laserdesk::rtc::RtcStatus>(st));
  const auto& s = std::get<laserdesk::rtc::RtcStatus>(st);
  EXPECT_EQ(s.rtc_mode, "ethernet");
  ASSERT_TRUE(s.remote_status_register.has_value());

  rtc.disconnect();
}
