#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace laserdesk::rtc::rif {

/// UDP send + wait for one datagram reply (RTC6 Remote Interface, manual §16.10.8).
class UdpRifChannel {
 public:
  UdpRifChannel() = default;
  UdpRifChannel(const UdpRifChannel&) = delete;
  UdpRifChannel& operator=(const UdpRifChannel&) = delete;

  /// Opens IPv4 UDP socket, binds ephemeral local port, sets `remote_host`:`remote_port`.
  std::string open(const std::string& remote_host, std::uint16_t remote_port);

  void close() noexcept;

  /// Sends `packet` and blocks up to `timeout_ms` for one UDP datagram (caller should validate sender).
  std::string request_response(const std::vector<std::uint8_t>& packet, int timeout_ms,
                                std::vector<std::uint8_t>& response_out) const;

  bool is_open() const noexcept { return open_; }

 private:
  bool open_{false};
  std::string remote_host_;
  std::uint16_t remote_port_{0};
};

}  // namespace laserdesk::rtc::rif
