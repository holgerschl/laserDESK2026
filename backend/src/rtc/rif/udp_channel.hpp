#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace laserdesk::rtc::rif {

/// UDP send + wait for reply datagram(s) (RTC6 Remote Interface, manual §16.10.8).
class UdpRifChannel {
 public:
  UdpRifChannel() = default;
  UdpRifChannel(const UdpRifChannel&) = delete;
  UdpRifChannel& operator=(const UdpRifChannel&) = delete;

  /// Opens channel state for `remote_host`:`remote_port`. UDP socket is created per request.
  /// If `local_bind_host` is non-empty, each operation binds the local endpoint to that IPv4 address.
  std::string open(const std::string& remote_host, std::uint16_t remote_port,
                   const std::string& local_bind_host = {});

  void close() noexcept;

  /// Sends `packet` and blocks up to `timeout_ms` for one UDP datagram from the configured remote only.
  std::string request_response(const std::vector<std::uint8_t>& packet, int timeout_ms,
                                std::vector<std::uint8_t>& response_out) const;

  /// Sends `packet` and reads until an Answer telegram matches `expect_seq` and `expect_format`, or deadline.
  /// Discards up to `max_extra_datagrams` other datagrams from the same remote (reordering).
  /// If `spurious_skipped_out` is non-null, adds the number of discarded datagrams.
  std::string request_response_matching(const std::vector<std::uint8_t>& packet, int timeout_ms,
                                        std::uint32_t expect_seq, std::uint32_t expect_format,
                                        int max_extra_datagrams, std::vector<std::uint8_t>& response_out,
                                        std::uint64_t* spurious_skipped_out = nullptr) const;

  bool is_open() const noexcept { return open_; }

 private:
  bool open_{false};
  std::string remote_host_;
  std::uint16_t remote_port_{0};
  std::string local_bind_host_;
};

}  // namespace laserdesk::rtc::rif
