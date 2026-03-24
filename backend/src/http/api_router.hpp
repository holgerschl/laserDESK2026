#pragma once

#include "rtc/rtc_client.hpp"

#include <httplib.h>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace laserdesk::http_api {

/// Thread-safe session: at most one RTC client (mock or future ethernet).
class BackendSession {
 public:
  std::string health_rtc_mode() const;

  nlohmann::json handle_get_health() const;
  nlohmann::json handle_get_rtc_status() const;
  /// Outbound RIF command log (Ethernet only; mock returns empty `lines`).
  nlohmann::json handle_get_rtc_rif_log() const;

  /// 204 on success; error body + status on failure
  int handle_post_rtc_connect(const nlohmann::json& body, nlohmann::json& err_out);
  int handle_post_rtc_disconnect();
  /// `R_DC_STOP_EXECUTION` — same as job stop handlers; no job id required.
  int handle_post_rtc_stop(nlohmann::json& err_out);

  /// Multipart: field `file` (binary). Optional form fields: table_no, dim, head_a, head_b, number_of_tables.
  int handle_post_rtc_correction_load(const httplib::Request& req, nlohmann::json& err_out);

  int handle_post_minimal_demo_job(const nlohmann::json& body, nlohmann::json& out,
                                   nlohmann::json& err_out);
  int handle_post_minimal_demo_run(const httplib::Request& req, nlohmann::json& err_out);
  int handle_post_minimal_demo_stop(nlohmann::json& err_out);

  int handle_post_jobs_dxf(const httplib::Request& req, nlohmann::json& out, nlohmann::json& err_out);
  int handle_post_jobs_scene(const nlohmann::json& body, nlohmann::json& out, nlohmann::json& err_out);
  int handle_get_jobs_dxf(const std::string& job_id, nlohmann::json& out, nlohmann::json& err_out);
  int handle_post_jobs_dxf_load(const std::string& job_id, nlohmann::json& err_out);
  int handle_post_jobs_dxf_run(const httplib::Request& req, nlohmann::json& err_out);
  int handle_post_jobs_dxf_stop(nlohmann::json& err_out);

  /// Pre-connect mock for --rtc-demo
  void auto_demo_connect_mock();

 private:
  /// Caller must hold `mutex_`.
  int rtc_stop_execution_locked(nlohmann::json& err_out);

  mutable std::mutex mutex_;
  std::unique_ptr<rtc::IRtcClient> rtc_;
  std::unordered_map<std::string, nlohmann::json> dxf_jobs_;
};

void register_api_routes(httplib::Server& svr, BackendSession& session);

}  // namespace laserdesk::http_api
