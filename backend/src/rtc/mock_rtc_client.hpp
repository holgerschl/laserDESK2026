#pragma once

#include "rtc_client.hpp"

#include <mutex>
#include <string>

namespace laserdesk::rtc {

class MockRtcClient final : public IRtcClient {
 public:
  MockRtcClient() = default;

  std::optional<RtcError> connect(const RtcConnectConfig& cfg) override;
  void disconnect() override;
  std::variant<RtcStatus, RtcError> get_status() const override;
  std::variant<std::string, RtcError> load_minimal_job(const std::string& label) override;
  std::optional<RtcError> start_execution() override;
  std::optional<RtcError> stop_execution() override;

 private:
  enum class State {
    Disconnected,
    ConnectedIdle,
    Loaded,
    Running,
    Error
  };

  static RtcError err(std::string code, std::string message);
  RtcStatus build_status() const;

  mutable std::mutex mutex_;
  State state_{State::Disconnected};
  std::string current_label_;
  std::string current_job_id_;
  std::optional<RtcError> latched_error_;
};

}  // namespace laserdesk::rtc
