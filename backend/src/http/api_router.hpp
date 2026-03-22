#pragma once

#include "rtc/rtc_client.hpp"

#include <httplib.h>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

namespace laserdesk::http_api {

/// Thread-safe session: at most one RTC client (mock or future ethernet).
class BackendSession {
 public:
  std::string health_rtc_mode() const;

  nlohmann::json handle_get_health() const;
  nlohmann::json handle_get_rtc_status() const;

  /// 204 on success; error body + status on failure
  int handle_post_rtc_connect(const nlohmann::json& body, nlohmann::json& err_out);
  int handle_post_rtc_disconnect();

  int handle_post_minimal_demo_job(const nlohmann::json& body, nlohmann::json& out,
                                   nlohmann::json& err_out);
  int handle_post_minimal_demo_run(nlohmann::json& err_out);
  int handle_post_minimal_demo_stop(nlohmann::json& err_out);

  /// Pre-connect mock for --rtc-demo
  void auto_demo_connect_mock();

 private:
  mutable std::mutex mutex_;
  std::unique_ptr<rtc::IRtcClient> rtc_;
};

void register_api_routes(httplib::Server& svr, BackendSession& session);

}  // namespace laserdesk::http_api
