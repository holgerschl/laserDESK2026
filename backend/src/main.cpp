#include "http/api_router.hpp"

#include <httplib.h>

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void print_usage(const char* argv0) {
  std::cerr << "Usage: " << argv0 << " [--port N] [--rtc-demo]\n"
            << "  --port N      Listen port (default 8080 or LASERDESK_PORT)\n"
            << "  --rtc-demo    Auto-connect mock RTC on startup\n"
            << "Environment:\n"
            << "  LASERDESK_CORS_ORIGIN   e.g. https://user.github.io (HTTPS UI → localhost; enables PNA headers)\n";
}

int parse_args(int argc, char** argv, int& port, bool& rtc_demo) {
  port = 0;
  rtc_demo = false;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--help" || a == "-h") {
      return -1;
    }
    if (a == "--rtc-demo") {
      rtc_demo = true;
      continue;
    }
    if (a == "--port" && i + 1 < argc) {
      port = std::atoi(argv[++i]);
      continue;
    }
    std::cerr << "Unknown argument: " << a << "\n";
    return -1;
  }
  if (port <= 0) {
    const char* env = std::getenv("LASERDESK_PORT");
    port = env ? std::atoi(env) : 8080;
  }
  if (port <= 0 || port > 65535) {
    std::cerr << "Invalid port\n";
    return -1;
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  int port = 8080;
  bool rtc_demo = false;
  if (parse_args(argc, argv, port, rtc_demo) < 0) {
    print_usage(argv[0]);
    return argc > 1 ? 1 : 0;
  }

  httplib::Server svr;

  const char* cors = std::getenv("LASERDESK_CORS_ORIGIN");
  if (cors && cors[0] != '\0') {
    httplib::Headers hdrs;
    hdrs.emplace("Access-Control-Allow-Origin", cors);
    hdrs.emplace("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    hdrs.emplace("Access-Control-Allow-Headers", "Content-Type, Access-Control-Request-Private-Network");
    hdrs.emplace("Access-Control-Allow-Private-Network", "true");
    hdrs.emplace("Access-Control-Max-Age", "86400");
    svr.set_default_headers(std::move(hdrs));
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) { res.status = 204; });
    std::cerr << "[laserdesk_backend] CORS: Access-Control-Allow-Origin=" << cors << "\n";
    std::cerr << "[laserdesk_backend] CORS: Access-Control-Allow-Private-Network=true (required for HTTPS "
                 "pages → localhost in Chromium)\n";
  }

  laserdesk::http_api::BackendSession session;
  if (rtc_demo) {
    session.auto_demo_connect_mock();
    std::cerr << "[laserdesk_backend] --rtc-demo: mock RTC pre-connected\n";
  }

  laserdesk::http_api::register_api_routes(svr, session);

  std::cerr << "[laserdesk_backend] listening on 0.0.0.0:" << port << "\n";
  if (!svr.listen("0.0.0.0", port)) {
    std::cerr << "[laserdesk_backend] failed to bind port " << port << "\n";
    return 1;
  }
  return 0;
}
