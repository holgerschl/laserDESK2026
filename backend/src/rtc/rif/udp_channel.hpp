#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace laserdesk::rtc::rif {

namespace detail {
struct UdpRifChannelImpl;
}

/// UDP send + wait for reply datagram(s) (RTC6 Remote Interface, manual §16.10.8).
/// Uses one bound local socket for the whole session (same as SCANLAB `NetworkAdapter` usage).
class UdpRifChannel {
 public:
  UdpRifChannel();
  ~UdpRifChannel();
  UdpRifChannel(UdpRifChannel&&) noexcept;
  UdpRifChannel& operator=(UdpRifChannel&&) noexcept;
  UdpRifChannel(const UdpRifChannel&) = delete;
  UdpRifChannel& operator=(const UdpRifChannel&) = delete;

  /// Creates IPv4 UDP socket, binds local endpoint, resolves `remote_host`:`remote_port`.
  /// If `local_bind_host` is non-empty, bind to that IPv4 address (port 0 = ephemeral).
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

  bool is_open() const noexcept;

 private:
  bool open_{false};
  mutable std::unique_ptr<detail::UdpRifChannelImpl> impl_;
};

}  // namespace laserdesk::rtc::rif
