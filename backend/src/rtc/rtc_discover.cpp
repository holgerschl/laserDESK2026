#include "rtc_discover.hpp"

#include "rif/telegram_raw.hpp"
#include "rif/udp_channel.hpp"

#include <asio/ip/address_v4.hpp>

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace laserdesk::rtc {

namespace {

bool parse_ipv4(const std::string& s, std::uint32_t& out_host_order, std::string& err) {
  asio::error_code ec;
  auto a = asio::ip::make_address_v4(s, ec);
  if (ec) {
    err = "Invalid IPv4 address: " + s;
    return false;
  }
  out_host_order = a.to_uint();
  return true;
}

bool probe_rtc_at(const std::string& ip, std::uint16_t port, std::uint32_t tgm_format, int timeout_ms,
                  nlohmann::json& host_obj) {
  constexpr std::uint32_t kSeq = 1u;
  std::vector<std::uint8_t> pkt =
      rif::build_command_telegram(kSeq, tgm_format, {rif::kRdcGetStatus});
  rif::UdpRifChannel udp;
  if (!udp.open(ip, port).empty()) return false;
  std::vector<std::uint8_t> raw;
  std::string io = udp.request_response(pkt, timeout_ms, raw);
  if (!io.empty()) return false;
  rif::ParsedAnswer ans = rif::parse_answer_telegram(raw.data(), raw.size(), kSeq, tgm_format);
  if (!ans.ok || ans.pl_words.size() < 4u) return false;
  if (ans.pl_words[1] != rif::kRdcGetStatus) return false;
  if (ans.last_error != 0u) return false;
  host_obj["ip"] = ip;
  host_obj["remote_status"] = ans.pl_words[2];
  host_obj["remote_pos"] = ans.pl_words[3];
  return true;
}

std::string ipv4_to_string(std::uint32_t host_order) {
  return asio::ip::address_v4(host_order).to_string();
}

}  // namespace

int handle_rtc_discover_json(const nlohmann::json& body, nlohmann::json& out) {
  const std::string base_ip = body.value("base_ip", "");
  const std::string netmask = body.value("netmask", "");
  if (base_ip.empty() || netmask.empty()) {
    out = nlohmann::json{{"code", "RTC_INTERNAL"}, {"message", "base_ip and netmask are required (IPv4 dotted)"}};
    return 400;
  }

  std::uint32_t base_u = 0;
  std::uint32_t mask_u = 0;
  std::string perr;
  if (!parse_ipv4(base_ip, base_u, perr)) {
    out = nlohmann::json{{"code", "RTC_INTERNAL"}, {"message", perr}};
    return 400;
  }
  if (!parse_ipv4(netmask, mask_u, perr)) {
    out = nlohmann::json{{"code", "RTC_INTERNAL"}, {"message", "netmask: " + perr}};
    return 400;
  }

  const int port = body.contains("port") && body["port"].is_number_integer() ? body["port"].get<int>() : 5020;
  if (port <= 0 || port > 65535) {
    out = nlohmann::json{{"code", "RTC_INTERNAL"}, {"message", "invalid port"}};
    return 400;
  }
  const std::uint32_t tgm_format =
      body.contains("tgm_format") && body["tgm_format"].is_number_unsigned() ? body["tgm_format"].get<std::uint32_t>()
                                                                              : 0u;
  int timeout_ms = body.contains("timeout_ms") && body["timeout_ms"].is_number_integer() ? body["timeout_ms"].get<int>()
                                                                                       : 120;
  if (timeout_ms < 30) timeout_ms = 30;
  if (timeout_ms > 2000) timeout_ms = 2000;

  std::size_t max_hosts = body.contains("max_hosts") && body["max_hosts"].is_number_unsigned()
                              ? body["max_hosts"].get<std::size_t>()
                              : 512u;
  if (max_hosts < 1) max_hosts = 1;
  if (max_hosts > 4096) max_hosts = 4096;

  const std::uint32_t net_u = base_u & mask_u;
  const std::uint32_t wildcard = ~mask_u;

  std::vector<std::string> candidates;
  if (wildcard == 0u) {
    candidates.push_back(ipv4_to_string(base_u));
  } else {
    for (std::uint32_t off = 1; off < wildcard && candidates.size() < max_hosts; ++off) {
      const std::uint32_t addr_u = net_u + off;
      if ((addr_u & mask_u) != net_u) break;
      candidates.push_back(ipv4_to_string(addr_u));
    }
  }

  const auto t0 = std::chrono::steady_clock::now();
  nlohmann::json hosts = nlohmann::json::array();
  std::size_t scanned = 0;
  for (const std::string& ip : candidates) {
    if (scanned >= max_hosts) break;
    nlohmann::json one;
    if (probe_rtc_at(ip, static_cast<std::uint16_t>(port), tgm_format, timeout_ms, one)) hosts.push_back(std::move(one));
    ++scanned;
  }
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();

  out = nlohmann::json::object();
  out["hosts"] = std::move(hosts);
  out["scanned"] = scanned;
  out["elapsed_ms"] = ms;
  return 200;
}

}  // namespace laserdesk::rtc
