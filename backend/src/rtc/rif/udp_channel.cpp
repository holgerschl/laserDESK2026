#include "udp_channel.hpp"

#include <asio.hpp>

#include <array>
#include <chrono>
#include <cstddef>

namespace laserdesk::rtc::rif {

std::string UdpRifChannel::open(const std::string& remote_host, std::uint16_t remote_port) {
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
  open_ = true;
  return {};
}

void UdpRifChannel::close() noexcept {
  open_ = false;
  remote_host_.clear();
  remote_port_ = 0;
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
    socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));

    asio::ip::udp::resolver resolver(io);
    asio::ip::udp::resolver::results_type endpoints =
        resolver.resolve(asio::ip::udp::v4(), remote_host_, std::to_string(remote_port_));
    if (endpoints.empty()) return "UDP resolve failed";
    asio::ip::udp::endpoint remote = *endpoints.begin();

    socket.send_to(asio::buffer(packet), remote);

    std::array<std::uint8_t, 2048> buf{};
    asio::ip::udp::endpoint sender;
    bool recv_done = false;
    std::string err;

    asio::steady_timer timer(io);
    timer.expires_after(std::chrono::milliseconds(timeout_ms));

    socket.async_receive_from(asio::buffer(buf), sender,
                              [&](const asio::error_code& ec, std::size_t n) {
                                timer.cancel();
                                if (ec) {
                                  if (ec != asio::error::operation_aborted) err = ec.message();
                                  recv_done = true;
                                  return;
                                }
                                response_out.assign(buf.begin(),
                                                    buf.begin() + static_cast<std::ptrdiff_t>(n));
                                recv_done = true;
                              });

    timer.async_wait([&](const asio::error_code& ec) {
      if (!ec) {
        socket.cancel();
        if (!recv_done) err = "UDP receive timeout";
      }
    });

    io.run();

    if (!err.empty()) return err;
    if (response_out.empty()) return "Empty UDP response";
    return {};
  } catch (const std::exception& e) {
    return std::string("UDP I/O error: ") + e.what();
  }
}

}  // namespace laserdesk::rtc::rif
