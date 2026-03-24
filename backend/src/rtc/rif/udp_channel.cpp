#include "udp_channel.hpp"

#include "telegram_raw.hpp"

#include <asio.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>

namespace laserdesk::rtc::rif {

namespace {

using clock = std::chrono::steady_clock;

/// One `async_receive_from` with timer; returns empty on success and fills `n_out` / `response_out` slice.
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

}  // namespace

std::string UdpRifChannel::open(const std::string& remote_host, std::uint16_t remote_port,
                                const std::string& local_bind_host) {
  close();
  try {
    asio::io_context io;
    asio::ip::udp::resolver resolver(io);
    asio::ip::udp::resolver::results_type results =
        resolver.resolve(asio::ip::udp::v4(), remote_host, std::to_string(remote_port));
    if (results.empty()) return "UDP resolve failed: no endpoints";
  } catch (const std::exception& e) {
    return std::string("UDP resolve error: ") + e.what();
  }
  remote_host_ = remote_host;
  remote_port_ = remote_port;
  local_bind_host_ = local_bind_host;
  open_ = true;
  return {};
}

void UdpRifChannel::close() noexcept {
  open_ = false;
  remote_host_.clear();
  remote_port_ = 0;
  local_bind_host_.clear();
}

std::string UdpRifChannel::request_response(const std::vector<std::uint8_t>& packet, int timeout_ms,
                                            std::vector<std::uint8_t>& response_out) const {
  response_out.clear();
  if (!open_) return "UDP channel not open";
  if (timeout_ms <= 0) timeout_ms = 500;

  try {
    asio::io_context io;
    asio::ip::udp::socket socket(io);
    socket.open(asio::ip::udp::v4());
    if (local_bind_host_.empty()) {
      socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));
    } else {
      asio::error_code bec;
      auto addr = asio::ip::make_address(local_bind_host_, bec);
      if (bec) return std::string("UDP local bind: invalid address: ") + bec.message();
      socket.bind(asio::ip::udp::endpoint(addr, 0));
    }

    asio::ip::udp::resolver resolver(io);
    asio::ip::udp::resolver::results_type endpoints =
        resolver.resolve(asio::ip::udp::v4(), remote_host_, std::to_string(remote_port_));
    if (endpoints.empty()) return "UDP resolve failed";
    asio::ip::udp::endpoint remote = *endpoints.begin();

    socket.send_to(asio::buffer(packet), remote);

    std::array<std::uint8_t, 2048> buf{};
    asio::ip::udp::endpoint sender;
    std::size_t n = 0;
    std::string err = receive_one(io, socket, remote, buf, sender, timeout_ms, n);
    if (!err.empty()) return err;
    if (sender != remote) return "UDP reply from unexpected endpoint";
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
  if (!open_) return "UDP channel not open";
  if (timeout_ms <= 0) timeout_ms = 500;
  if (max_extra_datagrams < 0) max_extra_datagrams = 0;

  try {
    asio::io_context io;
    asio::ip::udp::socket socket(io);
    socket.open(asio::ip::udp::v4());
    if (local_bind_host_.empty()) {
      socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));
    } else {
      asio::error_code bec;
      auto addr = asio::ip::make_address(local_bind_host_, bec);
      if (bec) return std::string("UDP local bind: invalid address: ") + bec.message();
      socket.bind(asio::ip::udp::endpoint(addr, 0));
    }

    asio::ip::udp::resolver resolver(io);
    asio::ip::udp::resolver::results_type endpoints =
        resolver.resolve(asio::ip::udp::v4(), remote_host_, std::to_string(remote_port_));
    if (endpoints.empty()) return "UDP resolve failed";
    asio::ip::udp::endpoint remote = *endpoints.begin();

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
      if (sender != remote) return "UDP reply from unexpected endpoint";
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
