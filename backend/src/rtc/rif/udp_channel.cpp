#include "udp_channel.hpp"

#include "telegram_raw.hpp"

#include <asio.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <optional>
#include <utility>

namespace laserdesk::rtc::rif {

namespace detail {

struct UdpRifChannelImpl {
  asio::io_context io;
  std::optional<asio::ip::udp::socket> sock;
  asio::ip::udp::endpoint remote{};
};

}  // namespace detail

namespace {

using clock = std::chrono::steady_clock;

std::string receive_one(asio::io_context& io, asio::ip::udp::socket& socket,
                        const asio::ip::udp::endpoint& remote, std::array<std::uint8_t, 2048>& buf,
                        asio::ip::udp::endpoint& sender, int timeout_ms_slice, std::size_t& n_out) {
  n_out = 0;
  if (timeout_ms_slice <= 0) timeout_ms_slice = 1;

  bool recv_done = false;
  std::string err;

  asio::steady_timer timer(io);
  timer.expires_after(std::chrono::milliseconds(timeout_ms_slice));

  socket.async_receive_from(asio::buffer(buf), sender,
                            [&](const asio::error_code& ec, std::size_t n) {
                              timer.cancel();
                              if (ec) {
                                if (ec != asio::error::operation_aborted) err = ec.message();
                                recv_done = true;
                                return;
                              }
                              n_out = n;
                              recv_done = true;
                            });

  timer.async_wait([&](const asio::error_code& ec) {
    if (!ec) {
      socket.cancel();
      if (!recv_done) err = "UDP receive timeout";
    }
  });

  io.run();
  io.restart();
  return err;
}

bool sender_matches_remote(const asio::ip::udp::endpoint& sender, const asio::ip::udp::endpoint& remote) {
  // Match board IPv4 only; some firmware paths may use a different source UDP port than the service port.
  return sender.address() == remote.address();
}

}  // namespace

UdpRifChannel::UdpRifChannel() = default;
UdpRifChannel::~UdpRifChannel() = default;
UdpRifChannel::UdpRifChannel(UdpRifChannel&&) noexcept = default;
UdpRifChannel& UdpRifChannel::operator=(UdpRifChannel&&) noexcept = default;

bool UdpRifChannel::is_open() const noexcept {
  return open_ && impl_ && impl_->sock.has_value();
}

std::string UdpRifChannel::open(const std::string& remote_host, std::uint16_t remote_port,
                                const std::string& local_bind_host) {
  close();
  try {
    auto impl = std::make_unique<detail::UdpRifChannelImpl>();
    asio::ip::udp::resolver resolver(impl->io);
    asio::ip::udp::resolver::results_type results =
        resolver.resolve(asio::ip::udp::v4(), remote_host, std::to_string(remote_port));
    if (results.empty()) return "UDP resolve failed: no endpoints";
    impl->remote = *results.begin();

    impl->sock.emplace(impl->io);
    impl->sock->open(asio::ip::udp::v4());
    if (local_bind_host.empty()) {
      impl->sock->bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));
    } else {
      asio::error_code bec;
      auto addr = asio::ip::make_address(local_bind_host, bec);
      if (bec) return std::string("UDP local bind: invalid address: ") + bec.message();
      impl->sock->bind(asio::ip::udp::endpoint(addr, 0));
    }

    impl_ = std::move(impl);
    open_ = true;
    return {};
  } catch (const std::exception& e) {
    return std::string("UDP open error: ") + e.what();
  }
}

void UdpRifChannel::close() noexcept {
  impl_.reset();
  open_ = false;
}

std::string UdpRifChannel::request_response(const std::vector<std::uint8_t>& packet, int timeout_ms,
                                            std::vector<std::uint8_t>& response_out) const {
  response_out.clear();
  if (!is_open()) return "UDP channel not open";
  if (timeout_ms <= 0) timeout_ms = 500;

  try {
    auto& io = impl_->io;
    auto& socket = *impl_->sock;
    const auto& remote = impl_->remote;

    socket.send_to(asio::buffer(packet), remote);

    std::array<std::uint8_t, 2048> buf{};
    asio::ip::udp::endpoint sender;
    std::size_t n = 0;
    std::string err = receive_one(io, socket, remote, buf, sender, timeout_ms, n);
    if (!err.empty()) return err;
    if (!sender_matches_remote(sender, remote)) return "UDP reply from unexpected endpoint";
    if (n == 0) return "Empty UDP response";
    response_out.assign(buf.begin(), buf.begin() + static_cast<std::ptrdiff_t>(n));
    return {};
  } catch (const std::exception& e) {
    return std::string("UDP I/O error: ") + e.what();
  }
}

std::string UdpRifChannel::request_response_matching(const std::vector<std::uint8_t>& packet, int timeout_ms,
                                                     std::uint32_t expect_seq, std::uint32_t expect_format,
                                                     int max_extra_datagrams,
                                                     std::vector<std::uint8_t>& response_out,
                                                     std::uint64_t* spurious_skipped_out) const {
  response_out.clear();
  if (!is_open()) return "UDP channel not open";
  if (timeout_ms <= 0) timeout_ms = 500;
  if (max_extra_datagrams < 0) max_extra_datagrams = 0;

  try {
    auto& io = impl_->io;
    auto& socket = *impl_->sock;
    const auto& remote = impl_->remote;

    socket.send_to(asio::buffer(packet), remote);

    const clock::time_point deadline = clock::now() + std::chrono::milliseconds(timeout_ms);
    int extra_used = 0;
    std::uint64_t spurious = 0;

    std::array<std::uint8_t, 2048> buf{};
    asio::ip::udp::endpoint sender;

    for (;;) {
      const auto now = clock::now();
      if (now >= deadline) return "UDP receive timeout";
      const int slice = static_cast<int>(
          std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count());
      std::size_t n = 0;
      std::string err = receive_one(io, socket, remote, buf, sender, std::max(1, slice), n);
      if (!err.empty()) return err;
      if (!sender_matches_remote(sender, remote)) return "UDP reply from unexpected endpoint";
      if (n == 0) return "Empty UDP response";

      if (answer_raw_matches_seq_and_format(buf.data(), n, expect_seq, expect_format)) {
        response_out.assign(buf.begin(), buf.begin() + static_cast<std::ptrdiff_t>(n));
        if (spurious_skipped_out) *spurious_skipped_out += spurious;
        return {};
      }

      if (extra_used >= max_extra_datagrams) {
        return "No matching answer telegram (datagram cap exceeded)";
      }
      extra_used++;
      spurious++;
    }
  } catch (const std::exception& e) {
    return std::string("UDP I/O error: ") + e.what();
  }
}

}  // namespace laserdesk::rtc::rif
