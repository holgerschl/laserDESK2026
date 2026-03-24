#include "command_log_format.hpp"

#include <cmath>
#include <cstring>
#include <limits>
#include <sstream>

namespace laserdesk::rtc::rif {
namespace {

double try_double_at(const std::vector<std::uint32_t>& words, std::size_t idx) {
  if (idx + 2 > words.size()) return std::numeric_limits<double>::quiet_NaN();
  std::uint32_t parts[2] = {words[idx], words[idx + 1]};
  double d;
  std::memcpy(&d, parts, sizeof(d));
  return d;
}

const char* dc_name(std::uint32_t id) {
  switch (id) {
    case 1u:
      return "R_DC_CONFIG_LIST";
    case 3u:
      return "R_DC_SET_START_LIST_POS";
    case 4u:
      return "R_DC_GET_INPUT_POINTER";
    case 6u:
      return "R_DC_LOAD_LIST_POS";
    case 11u:
      return "R_DC_SET_MAX_COUNT";
    case 15u:
      return "R_DC_EXECUTE_LIST_POS";
    case 16u:
      return "R_DC_STOP_EXECUTION";
    case 31u:
      return "R_DC_GET_STATUS";
    case 38u:
      return "R_DC_GET_HEAD_PARA";
    case 71u:
      return "R_DC_SET_JUMP_SPEED";
    case 72u:
      return "R_DC_SET_MARK_SPEED";
    case 130u:
      return "R_DC_SELECT_COR_TABLE";
    case 142u:
      return "R_DC_GET_RTC_VERSION";
    case 143u:
      return "R_DC_GET_BIOS_VERSION";
    case 154u:
      return "R_DC_LOAD_CORRECTION_FILE";
    case 155u:
      return "R_DC_NUMBER_OF_COR_TABLES";
    default:
      return nullptr;
  }
}

const char* lc_name(std::uint32_t id) {
  switch (id) {
    case 259u:
      return "R_LC_END_OF_LIST";
    case 301u:
      return "R_LC_SET_MARK_SPEED";
    case 302u:
      return "R_LC_SET_JUMP_SPEED";
    case 312u:
      return "R_LC_MARK_XYZT_ABS";
    case 321u:
      return "R_LC_JUMP_XY_ABS";
    default:
      return nullptr;
  }
}

}  // namespace

std::string format_rif_command_log_line(std::uint32_t seq, const std::vector<std::uint32_t>& words) {
  std::ostringstream os;
  os << "seq=" << seq;
  if (words.empty()) {
    os << " (empty payload)";
    return os.str();
  }
  const std::uint32_t id = words[0];
  if (id == 0x12345678u) {
    os << " RIF_SEQ_SYNC payload=0x12345678";
    return os.str();
  }
  const char* nm = dc_name(id);
  if (!nm) nm = lc_name(id);
  if (nm) {
    os << ' ' << nm << " (" << id << ')';
  } else {
    os << " RIF_CMD (" << id << ')';
  }
  os << " u32=[";
  for (std::size_t i = 0; i < words.size(); ++i) {
    if (i) os << ',';
    os << words[i];
  }
  os << ']';
  if ((id == 71u || id == 72u || id == 301u || id == 302u) && words.size() >= 3u) {
    const double f = try_double_at(words, 1u);
    if (std::isfinite(f)) os << " f=" << f;
  }
  return os.str();
}

}  // namespace laserdesk::rtc::rif
