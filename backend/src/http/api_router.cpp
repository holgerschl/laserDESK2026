#include "api_router.hpp"

#include "dxf/ascii_dxf_lines.hpp"
#include "rtc/ethernet_rtc_client.hpp"
#include "rtc/rtc_discover.hpp"
#include "rtc/job_id.hpp"
#include "rtc/job/scene_v1.hpp"
#include "rtc/mock_rtc_client.hpp"

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

namespace laserdesk::http_api {

namespace {

nlohmann::json error_json(const rtc::RtcError& e) {
  return nlohmann::json{{"code", e.code}, {"message", e.message}};
}

constexpr std::size_t kMaxDxfUploadBytes = 20u * 1024u * 1024u;
constexpr std::size_t kMaxCorrectionFileBytes = 16u * 1024u * 1024u;

/// Optional query `repeat_count` for job run endpoints (1..1'000'000, default 1).
bool parse_job_run_repeat_count(const httplib::Request& req, std::uint32_t& out, std::string& bad_message) {
  out = 1u;
  if (!req.has_param("repeat_count")) return true;
  const std::string s = req.get_param_value("repeat_count");
  if (s.empty()) return true;
  char* end = nullptr;
  unsigned long v = std::strtoul(s.c_str(), &end, 10);
  if (end != s.c_str() + s.size()) {
    bad_message = "repeat_count must be a decimal integer";
    return false;
  }
  if (v == 0ul || v > 1'000'000ul) {
    bad_message = "repeat_count must be between 1 and 1000000";
    return false;
  }
  out = static_cast<std::uint32_t>(v);
  return true;
}

std::optional<std::string> read_file_all(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return std::nullopt;
  std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  return data;
}

std::optional<std::string> load_demo_dxf_text() {
  if (const char* env = std::getenv("LASERDESK_DEMO_DXF")) {
    if (auto s = read_file_all(env)) return s;
  }
  static constexpr const char* kCandidates[] = {
      "demo/dxf/SCANLABLogo.dxf",
      "../demo/dxf/SCANLABLogo.dxf",
      "../../demo/dxf/SCANLABLogo.dxf",
  };
  for (const char* p : kCandidates) {
    if (auto s = read_file_all(p)) return s;
  }
  return std::nullopt;
}

}  // namespace

std::string BackendSession::health_rtc_mode() const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!rtc_) return "disconnected";
  auto st = rtc_->get_status();
  if (std::holds_alternative<rtc::RtcError>(st)) return "disconnected";
  return std::get<rtc::RtcStatus>(st).rtc_mode;
}

nlohmann::json BackendSession::handle_get_health() const {
  nlohmann::json j{{"status", rtc::kHealthOk}};
  j["rtc_mode"] = health_rtc_mode();
  return j;
}

nlohmann::json BackendSession::handle_get_rtc_status() const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!rtc_) {
    return nlohmann::json{{"connection_state", "disconnected"}, {"rtc_mode", nullptr}};
  }
  auto st = rtc_->get_status();
  if (std::holds_alternative<rtc::RtcError>(st)) {
    const auto& e = std::get<rtc::RtcError>(st);
    return nlohmann::json{{"connection_state", "disconnected"},
                          {"last_error", error_json(e)}};
  }
  const auto& r = std::get<rtc::RtcStatus>(st);
  nlohmann::json j{{"connection_state", r.connection_state}, {"rtc_mode", r.rtc_mode}};
  if (r.package_version_reported) j["package_version_reported"] = *r.package_version_reported;
  if (r.bios_eth_reported) j["bios_eth_reported"] = *r.bios_eth_reported;
  j["alignment_ok"] = r.alignment_ok;
  if (r.remote_status_register) j["remote_status"] = *r.remote_status_register;
  if (r.remote_pos_register) j["remote_pos"] = *r.remote_pos_register;
  if (r.last_error) j["last_error"] = error_json(*r.last_error);
  if (r.active_dxf_line_count) j["dxf_line_count"] = *r.active_dxf_line_count;
  if (r.active_dxf_source_name) j["dxf_source_name"] = *r.active_dxf_source_name;
  if (r.correction_file_hint) j["correction_file_hint"] = *r.correction_file_hint;
  if (r.rif_udp_timeout_count) j["rif_udp_timeout_count"] = *r.rif_udp_timeout_count;
  if (r.rif_udp_spurious_datagrams) j["rif_udp_spurious_datagrams"] = *r.rif_udp_spurious_datagrams;
  if (r.rif_connect_status_retries_used) j["rif_connect_status_retries_used"] = *r.rif_connect_status_retries_used;
  return j;
}

int BackendSession::handle_post_rtc_connect(const nlohmann::json& body, nlohmann::json& err_out) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::string mode_s = body.value("mode", "");
  if (mode_s == "ethernet") {
    std::string host = body.value("host", "");
    if (host.empty()) {
      err_out = error_json(
          rtc::RtcError{"RTC_INTERNAL", "ethernet mode requires non-empty \"host\" (board IP or DNS)"});
      return 400;
    }
    int port = body.contains("port") && body["port"].is_number_integer() ? body["port"].get<int>() : 63750;
    if (port <= 0 || port > 65535) {
      err_out = error_json(rtc::RtcError{"RTC_INTERNAL", "invalid \"port\""});
      return 400;
    }
    rtc_ = rtc::make_ethernet_rtc_client();
    rtc::RtcConnectConfig cfg;
    cfg.mode = rtc::RtcConnectConfig::Mode::Ethernet;
    cfg.host = std::move(host);
    cfg.port = port;
    if (body.contains("tgm_format") && body["tgm_format"].is_number_unsigned())
      cfg.tgm_format = body["tgm_format"].get<std::uint32_t>();
    if (body.contains("recv_timeout_ms") && body["recv_timeout_ms"].is_number_integer())
      cfg.recv_timeout_ms = body["recv_timeout_ms"].get<int>();
    if (body.contains("udp_local_bind") && body["udp_local_bind"].is_string())
      cfg.udp_local_bind = body["udp_local_bind"].get<std::string>();
    if (body.contains("rif_connect_status_attempts") && body["rif_connect_status_attempts"].is_number_integer())
      cfg.rif_connect_status_attempts = body["rif_connect_status_attempts"].get<int>();
    if (body.contains("rif_udp_max_extra_datagrams") && body["rif_udp_max_extra_datagrams"].is_number_integer())
      cfg.rif_udp_max_extra_datagrams = body["rif_udp_max_extra_datagrams"].get<int>();
    if (body.contains("rif_retry_delay_ms") && body["rif_retry_delay_ms"].is_number_integer())
      cfg.rif_retry_delay_ms = body["rif_retry_delay_ms"].get<int>();
    if (body.contains("expected_package_tag") && body["expected_package_tag"].is_string())
      cfg.expected_package_tag = body["expected_package_tag"].get<std::string>();
    if (body.contains("expected_bios_eth_tag") && body["expected_bios_eth_tag"].is_string())
      cfg.expected_bios_eth_tag = body["expected_bios_eth_tag"].get<std::string>();
    if (body.contains("dxf_rif_list_upload") && body["dxf_rif_list_upload"].is_boolean())
      cfg.dxf_rif_list_upload = body["dxf_rif_list_upload"].get<bool>();
    if (body.contains("dxf_rif_bits_per_mm") && body["dxf_rif_bits_per_mm"].is_number())
      cfg.dxf_rif_bits_per_mm = body["dxf_rif_bits_per_mm"].get<double>();
    if (body.contains("rif_config_list_mem1") && body["rif_config_list_mem1"].is_number_unsigned())
      cfg.rif_config_list_mem1 = body["rif_config_list_mem1"].get<std::uint32_t>();
    if (body.contains("rif_config_list_mem2") && body["rif_config_list_mem2"].is_number_unsigned())
      cfg.rif_config_list_mem2 = body["rif_config_list_mem2"].get<std::uint32_t>();
    if (auto e = rtc_->connect(cfg)) {
      err_out = error_json(*e);
      rtc_.reset();
      return 503;
    }
    return 204;
  }
  if (mode_s != "mock") {
    err_out = error_json(
        rtc::RtcError{"RTC_INTERNAL", "Invalid or missing \"mode\" (use \"mock\" or \"ethernet\")"});
    return 400;
  }
  rtc_ = rtc::make_mock_rtc_client();
  rtc::RtcConnectConfig cfg;
  cfg.mode = rtc::RtcConnectConfig::Mode::Mock;
  if (auto e = rtc_->connect(cfg)) {
    err_out = error_json(*e);
    rtc_.reset();
    return 400;
  }
  return 204;
}

int BackendSession::handle_post_rtc_disconnect() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (rtc_) rtc_->disconnect();
  rtc_.reset();
  return 204;
}

namespace {

std::uint32_t parse_u32_param(const httplib::Request& req, const char* key, std::uint32_t default_v) {
  if (!req.has_param(key)) return default_v;
  const std::string s = req.get_param_value(key);
  if (s.empty()) return default_v;
  char* end = nullptr;
  unsigned long v = std::strtoul(s.c_str(), &end, 10);
  if (end == s.c_str() || v > 0xFFFFFFFFUL) return default_v;
  return static_cast<std::uint32_t>(v);
}

}  // namespace

int BackendSession::handle_post_rtc_correction_load(const httplib::Request& req, nlohmann::json& err_out) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!rtc_) {
    err_out = error_json(rtc::RtcError{"RTC_NOT_CONNECTED", "RTC session not established"});
    return 409;
  }
  if (!req.has_file("file")) {
    err_out = error_json(
        rtc::RtcError{"RTC_INTERNAL", "multipart form field \"file\" is required (.ct5 / correction data)"});
    return 400;
  }
  const httplib::MultipartFormData& f = req.get_file_value("file");
  if (f.content.size() > kMaxCorrectionFileBytes) {
    err_out = error_json(rtc::RtcError{"RTC_INTERNAL", "correction file too large (max 16 MiB)"});
    return 413;
  }
  std::vector<std::uint8_t> bytes(f.content.begin(), f.content.end());

  rtc::CorrectionFileLoadParams p;
  p.table_no = parse_u32_param(req, "table_no", 0u);
  p.dim = parse_u32_param(req, "dim", 2u);
  p.head_a = parse_u32_param(req, "head_a", 1u);
  p.head_b = parse_u32_param(req, "head_b", 1u);
  if (req.has_param("number_of_tables")) {
    const std::string nt = req.get_param_value("number_of_tables");
    if (!nt.empty()) p.number_of_tables = parse_u32_param(req, "number_of_tables", 1u);
  }
  if (req.has_param("finalize_arg3")) {
    const std::string fa = req.get_param_value("finalize_arg3");
    if (!fa.empty()) p.finalize_arg3 = parse_u32_param(req, "finalize_arg3", 0u);
  }

  if (auto e = rtc_->load_correction_file(bytes, p)) {
    err_out = error_json(*e);
    return 409;
  }
  return 204;
}

int BackendSession::handle_post_minimal_demo_job(const nlohmann::json& body, nlohmann::json& out,
                                                 nlohmann::json& err_out) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!rtc_) {
    err_out = error_json(rtc::RtcError{"RTC_NOT_CONNECTED", "RTC session not established"});
    return 409;
  }
  std::string label;
  if (body.contains("label") && body["label"].is_string()) label = body["label"].get<std::string>();
  auto res = rtc_->load_minimal_job(label);
  if (std::holds_alternative<rtc::RtcError>(res)) {
    err_out = error_json(std::get<rtc::RtcError>(res));
    return 409;
  }
  out = nlohmann::json{{"job_id", std::get<std::string>(res)}};
  return 200;
}

int BackendSession::handle_post_minimal_demo_run(const httplib::Request& req, nlohmann::json& err_out) {
  std::uint32_t repeats = 1u;
  std::string perr;
  if (!parse_job_run_repeat_count(req, repeats, perr)) {
    err_out = nlohmann::json{{"code", "RTC_INTERNAL"}, {"message", perr}};
    return 400;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (!rtc_) {
    err_out = error_json(rtc::RtcError{"RTC_NOT_CONNECTED", "RTC session not established"});
    return 409;
  }
  if (auto e = rtc_->start_execution(repeats)) {
    err_out = error_json(*e);
    return 409;
  }
  return 204;
}

int BackendSession::handle_post_minimal_demo_stop(nlohmann::json& err_out) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!rtc_) {
    err_out = error_json(rtc::RtcError{"RTC_NOT_CONNECTED", "RTC session not established"});
    return 409;
  }
  if (auto e = rtc_->stop_execution()) {
    // Mock RTC finishes lists synchronously (never leaves "running"); treat stop as idempotent.
    if (e->code == "RTC_NOT_RUNNING") return 204;
    err_out = error_json(*e);
    return 409;
  }
  return 204;
}

int BackendSession::handle_post_jobs_dxf(const httplib::Request& req, nlohmann::json& out,
                                           nlohmann::json& err_out) {
  std::string source_name = "dxf";
  std::string text;

  if (req.has_file("file")) {
    const httplib::MultipartFormData& f = req.get_file_value("file");
    source_name = f.filename.empty() ? "upload.dxf" : f.filename;
    text = f.content;
  } else {
    nlohmann::json body;
    try {
      body = nlohmann::json::parse(req.body.empty() ? "{}" : req.body);
    } catch (...) {
      err_out = error_json(rtc::RtcError{"DXF_PARSE_ERROR", "Invalid JSON body"});
      return 400;
    }
    if (body.value("source", "") == "demo") {
      auto demo = load_demo_dxf_text();
      if (!demo) {
        err_out = error_json(rtc::RtcError{
            "DXF_PARSE_ERROR",
            "Demo DXF not found. Set LASERDESK_DEMO_DXF or run from repo root (demo/dxf/SCANLABLogo.dxf)."});
        return 404;
      }
      text = std::move(*demo);
      source_name = "SCANLABLogo.dxf";
    } else if (body.contains("dxf_text") && body["dxf_text"].is_string()) {
      text = body["dxf_text"].get<std::string>();
      if (body.contains("source_name") && body["source_name"].is_string())
        source_name = body["source_name"].get<std::string>();
    } else {
      err_out = error_json(rtc::RtcError{
          "DXF_PARSE_ERROR", R"(Use {"source":"demo"}, {"dxf_text":"..."}[, "source_name"], or multipart file field "file")"});
      return 400;
    }
  }

  if (text.size() > kMaxDxfUploadBytes) {
    err_out = error_json(
        rtc::RtcError{"DXF_PARSE_ERROR", "DXF payload too large (max 20 MiB)"});
    return 413;
  }

  auto pr = dxf::parse_ascii_dxf_lines(source_name, text);
  if (pr.error_code) {
    err_out = nlohmann::json{{"code", *pr.error_code}, {"message", pr.error_message.value_or("")}};
    return 400;
  }

  const std::string job_id = rtc::make_demo_job_id();
  nlohmann::json doc = dxf::job_to_json(job_id, pr);

  std::lock_guard<std::mutex> lock(mutex_);
  dxf_jobs_[job_id] = doc;
  out = nlohmann::json{{"job_id", job_id}};
  return 200;
}

int BackendSession::handle_post_jobs_scene(const nlohmann::json& body, nlohmann::json& out,
                                             nlohmann::json& err_out) {
  dxf::ParseResult pr;
  std::string err;
  if (!rtc::job::scene_v1_to_parse_result(body, pr, err)) {
    err_out = nlohmann::json{{"code", "SCENE_INVALID"}, {"message", err}};
    return 400;
  }

  const std::string job_id = rtc::make_demo_job_id();
  nlohmann::json doc = dxf::job_to_json(job_id, pr);

  std::lock_guard<std::mutex> lock(mutex_);
  dxf_jobs_[job_id] = doc;
  out = nlohmann::json{{"job_id", job_id}};
  return 200;
}

int BackendSession::handle_get_jobs_dxf(const std::string& job_id, nlohmann::json& out,
                                        nlohmann::json& err_out) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = dxf_jobs_.find(job_id);
  if (it == dxf_jobs_.end()) {
    err_out = error_json(rtc::RtcError{"NOT_FOUND", "Unknown DXF job id"});
    return 404;
  }
  out = it->second;
  return 200;
}

int BackendSession::handle_post_jobs_dxf_load(const std::string& job_id, nlohmann::json& err_out) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!rtc_) {
    err_out = error_json(rtc::RtcError{"RTC_NOT_CONNECTED", "RTC session not established"});
    return 409;
  }
  auto it = dxf_jobs_.find(job_id);
  if (it == dxf_jobs_.end()) {
    err_out = error_json(rtc::RtcError{"NOT_FOUND", "Unknown DXF job id"});
    return 404;
  }
  if (auto e = rtc_->load_dxf_job(it->second)) {
    err_out = error_json(*e);
    return 409;
  }
  return 204;
}

int BackendSession::handle_post_jobs_dxf_run(const httplib::Request& req, nlohmann::json& err_out) {
  std::uint32_t repeats = 1u;
  std::string perr;
  if (!parse_job_run_repeat_count(req, repeats, perr)) {
    err_out = nlohmann::json{{"code", "RTC_INTERNAL"}, {"message", perr}};
    return 400;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (!rtc_) {
    err_out = error_json(rtc::RtcError{"RTC_NOT_CONNECTED", "RTC session not established"});
    return 409;
  }
  if (auto e = rtc_->start_execution(repeats)) {
    err_out = error_json(*e);
    return 409;
  }
  return 204;
}

int BackendSession::handle_post_jobs_dxf_stop(nlohmann::json& err_out) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!rtc_) {
    err_out = error_json(rtc::RtcError{"RTC_NOT_CONNECTED", "RTC session not established"});
    return 409;
  }
  if (auto e = rtc_->stop_execution()) {
    if (e->code == "RTC_NOT_RUNNING") return 204;
    err_out = error_json(*e);
    return 409;
  }
  return 204;
}

void BackendSession::auto_demo_connect_mock() {
  std::lock_guard<std::mutex> lock(mutex_);
  rtc_ = rtc::make_mock_rtc_client();
  rtc::RtcConnectConfig cfg;
  cfg.mode = rtc::RtcConnectConfig::Mode::Mock;
  rtc_->connect(cfg);
}

void register_api_routes(httplib::Server& svr, BackendSession& session) {
  svr.Get("/api/v1/health", [&](const httplib::Request&, httplib::Response& res) {
    res.set_content(session.handle_get_health().dump(), "application/json");
  });

  svr.Get("/api/v1/rtc/status", [&](const httplib::Request&, httplib::Response& res) {
    res.set_content(session.handle_get_rtc_status().dump(), "application/json");
  });

  svr.Post("/api/v1/rtc/connect", [&](const httplib::Request& req, httplib::Response& res) {
    nlohmann::json err;
    nlohmann::json body;
    try {
      body = nlohmann::json::parse(req.body.empty() ? "{}" : req.body);
    } catch (...) {
      err = error_json(rtc::RtcError{"RTC_INTERNAL", "Invalid JSON body"});
      res.status = 400;
      res.set_content(err.dump(), "application/json");
      return;
    }
    int code = session.handle_post_rtc_connect(body, err);
    res.status = code;
    if (code != 204) res.set_content(err.dump(), "application/json");
  });

  svr.Post("/api/v1/rtc/disconnect", [&](const httplib::Request&, httplib::Response& res) {
    res.status = session.handle_post_rtc_disconnect();
  });

  svr.Post("/api/v1/rtc/correction/load", [&](const httplib::Request& req, httplib::Response& res) {
    nlohmann::json err;
    int code = session.handle_post_rtc_correction_load(req, err);
    res.status = code;
    if (code != 204) res.set_content(err.dump(), "application/json");
  });

  svr.Post("/api/v1/rtc/discover", [&](const httplib::Request& req, httplib::Response& res) {
    nlohmann::json body;
    try {
      body = nlohmann::json::parse(req.body.empty() ? "{}" : req.body);
    } catch (...) {
      nlohmann::json err =
          nlohmann::json{{"code", "RTC_INTERNAL"}, {"message", "Invalid JSON body"}};
      res.status = 400;
      res.set_content(err.dump(), "application/json");
      return;
    }
    nlohmann::json out;
    res.status = rtc::handle_rtc_discover_json(body, out);
    res.set_content(out.dump(), "application/json");
  });

  svr.Post("/api/v1/jobs/minimal-demo", [&](const httplib::Request& req, httplib::Response& res) {
    nlohmann::json err, out;
    nlohmann::json body;
    try {
      body = req.body.empty() ? nlohmann::json::object() : nlohmann::json::parse(req.body);
    } catch (...) {
      err = error_json(rtc::RtcError{"RTC_INTERNAL", "Invalid JSON body"});
      res.status = 400;
      res.set_content(err.dump(), "application/json");
      return;
    }
    int code = session.handle_post_minimal_demo_job(body, out, err);
    res.status = code;
    res.set_content((code == 200 ? out : err).dump(), "application/json");
  });

  svr.Post("/api/v1/jobs/minimal-demo/run", [&](const httplib::Request& req, httplib::Response& res) {
    nlohmann::json err;
    int code = session.handle_post_minimal_demo_run(req, err);
    res.status = code;
    if (code != 204) res.set_content(err.dump(), "application/json");
  });

  svr.Post("/api/v1/jobs/minimal-demo/stop", [&](const httplib::Request&, httplib::Response& res) {
    nlohmann::json err;
    int code = session.handle_post_minimal_demo_stop(err);
    res.status = code;
    if (code != 204) res.set_content(err.dump(), "application/json");
  });

  svr.Post("/api/v1/jobs/dxf", [&](const httplib::Request& req, httplib::Response& res) {
    nlohmann::json err, out;
    int code = session.handle_post_jobs_dxf(req, out, err);
    res.status = code;
    res.set_content((code == 200 ? out : err).dump(), "application/json");
  });

  svr.Post("/api/v1/jobs/scene", [&](const httplib::Request& req, httplib::Response& res) {
    nlohmann::json err, out;
    nlohmann::json body;
    try {
      body = nlohmann::json::parse(req.body.empty() ? "{}" : req.body);
    } catch (...) {
      err = error_json(rtc::RtcError{"RTC_INTERNAL", "Invalid JSON body"});
      res.status = 400;
      res.set_content(err.dump(), "application/json");
      return;
    }
    int code = session.handle_post_jobs_scene(body, out, err);
    res.status = code;
    res.set_content((code == 200 ? out : err).dump(), "application/json");
  });

  svr.Get(R"(/api/v1/jobs/dxf/([^/]+)$)", [&](const httplib::Request& req, httplib::Response& res) {
    nlohmann::json err, out;
    int code = session.handle_get_jobs_dxf(req.matches[1], out, err);
    res.status = code;
    res.set_content((code == 200 ? out : err).dump(), "application/json");
  });

  svr.Post(R"(/api/v1/jobs/dxf/([^/]+)/load)", [&](const httplib::Request& req, httplib::Response& res) {
    nlohmann::json err;
    int code = session.handle_post_jobs_dxf_load(req.matches[1], err);
    res.status = code;
    if (code != 204) res.set_content(err.dump(), "application/json");
  });

  svr.Post(R"(/api/v1/jobs/dxf/([^/]+)/run)", [&](const httplib::Request& req, httplib::Response& res) {
    nlohmann::json err;
    int code = session.handle_post_jobs_dxf_run(req, err);
    res.status = code;
    if (code != 204) res.set_content(err.dump(), "application/json");
  });

  svr.Post(R"(/api/v1/jobs/dxf/([^/]+)/stop)", [&](const httplib::Request& req, httplib::Response& res) {
    nlohmann::json err;
    int code = session.handle_post_jobs_dxf_stop(err);
    res.status = code;
    if (code != 204) res.set_content(err.dump(), "application/json");
  });
}

}  // namespace laserdesk::http_api
