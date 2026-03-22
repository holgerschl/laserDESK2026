#pragma once

#include "rtc_client.hpp"
#include "rif/telegram_raw.hpp"
#include "rif/udp_channel.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace laserdesk::rtc {

class EthernetRtcClient final : public IRtcClient {
 public:
  EthernetRtcClient() = default;

  std::optional<RtcError> connect(const RtcConnectConfig& cfg) override;
  void disconnect() override;
  std::variant<RtcStatus, RtcError> get_status() const override;
  std::variant<std::string, RtcError> load_minimal_job(const std::string& label) override;
  std::optional<RtcError> load_dxf_job(const nlohmann::json& job_document) override;
  std::optional<RtcError> start_execution() override;
  std::optional<RtcError> stop_execution() override;

 private:
  enum class State { Disconnected, ConnectedIdle, Loaded, Running, Error };

  static RtcError err(std::string code, std::string message);
  RtcStatus build_status(const rif::ParsedAnswer* get_status_answer) const;

  std::optional<RtcError> send_remote_control(const std::vector<std::uint32_t>& words,
                                              rif::ParsedAnswer& out) const;

  mutable std::mutex mutex_;
  rif::UdpRifChannel udp_;
  State state_{State::Disconnected};
  /// Incremented on each UDP telegram (const get_status() may send).
  mutable std::uint32_t seq_{0};
  std::uint32_t format_{0};
  int timeout_ms_{800};
  std::string package_tag_;
  std::string bios_tag_;
  std::string last_job_label_;
  std::string last_job_id_;
  std::optional<std::size_t> dxf_line_count_;
  std::optional<std::string> dxf_source_name_;
  bool dxf_rif_list_upload_{false};
  double dxf_rif_bits_per_mm_{128.0};
  std::uint32_t rif_config_list_mem1_{1u};
  std::uint32_t rif_config_list_mem2_{2u};
};

}  // namespace laserdesk::rtc
