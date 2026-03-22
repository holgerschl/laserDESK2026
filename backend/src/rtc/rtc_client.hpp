#pragma once

#include "rtc_types.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <optional>
#include <string>
#include <variant>

namespace laserdesk::rtc {

class IRtcClient {
 public:
  virtual ~IRtcClient() = default;

  virtual std::optional<RtcError> connect(const RtcConnectConfig& cfg) = 0;
  virtual void disconnect() = 0;

  /// RTC_NOT_CONNECTED if not connected
  virtual std::variant<RtcStatus, RtcError> get_status() const = 0;

  virtual std::variant<std::string, RtcError> load_minimal_job(const std::string& label) = 0;
  /// Load parsed DXF job document (same shape as GET /jobs/dxf/{id}) into the RTC session.
  virtual std::optional<RtcError> load_dxf_job(const nlohmann::json& job_document) = 0;
  virtual std::optional<RtcError> start_execution() = 0;
  virtual std::optional<RtcError> stop_execution() = 0;
};

std::unique_ptr<IRtcClient> make_mock_rtc_client();
std::unique_ptr<IRtcClient> make_ethernet_rtc_client();

}  // namespace laserdesk::rtc
