#include "rtc/rtc_discover.hpp"

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

namespace {

using laserdesk::rtc::handle_rtc_discover_json;

TEST(RtcDiscover, MissingBaseIp) {
  nlohmann::json out;
  nlohmann::json body{{"netmask", "255.255.255.0"}};
  EXPECT_EQ(handle_rtc_discover_json(body, out), 400);
}

TEST(RtcDiscover, MissingNetmask) {
  nlohmann::json out;
  nlohmann::json body{{"base_ip", "192.168.1.1"}};
  EXPECT_EQ(handle_rtc_discover_json(body, out), 400);
}

TEST(RtcDiscover, InvalidIp) {
  nlohmann::json out;
  nlohmann::json body{{"base_ip", "not-an-ip"}, {"netmask", "255.255.255.0"}};
  EXPECT_EQ(handle_rtc_discover_json(body, out), 400);
}

TEST(RtcDiscover, SingleHostSlash32) {
  nlohmann::json out;
  nlohmann::json body{{"base_ip", "127.0.0.1"},
                      {"netmask", "255.255.255.255"},
                      {"timeout_ms", 30},
                      {"max_hosts", 1}};
  EXPECT_EQ(handle_rtc_discover_json(body, out), 200);
  EXPECT_TRUE(out.contains("hosts"));
  EXPECT_TRUE(out.contains("scanned"));
  EXPECT_TRUE(out.contains("elapsed_ms"));
  EXPECT_EQ(out["scanned"].get<std::size_t>(), 1u);
}

}  // namespace
