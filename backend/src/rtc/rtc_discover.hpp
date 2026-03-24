#pragma once

#include <nlohmann/json.hpp>

namespace laserdesk::rtc {

/// Scan IPv4 subnet with UDP R_DC_GET_STATUS probes (same telegram as ethernet connect handshake).
/// JSON body: base_ip (string), netmask (string, e.g. 255.255.255.0), optional port (default 63750),
/// tgm_format (default RAW = 1, see SCANLAB telegrams.h), timeout_ms (default 120), max_hosts (default 512, cap 4096).
/// Returns HTTP status 200 + { hosts, scanned, elapsed_ms } or 400 + error JSON.
int handle_rtc_discover_json(const nlohmann::json& body, nlohmann::json& out);

}  // namespace laserdesk::rtc
