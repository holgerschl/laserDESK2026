#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace laserdesk::rtc::rif {

/// One-line description for operator UI: seq, mnemonic, numeric payload words (decoded **f=** for IEEE doubles
/// on speed telegrams when applicable).
std::string format_rif_command_log_line(std::uint32_t seq, const std::vector<std::uint32_t>& words);

}  // namespace laserdesk::rtc::rif
