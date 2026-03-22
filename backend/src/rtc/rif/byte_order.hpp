#pragma once

#include <cstdint>
#include <vector>

namespace laserdesk::rtc::rif {

inline void write_u32_le(std::uint8_t* p, std::uint32_t v) noexcept {
  p[0] = static_cast<std::uint8_t>(v & 0xffu);
  p[1] = static_cast<std::uint8_t>((v >> 8) & 0xffu);
  p[2] = static_cast<std::uint8_t>((v >> 16) & 0xffu);
  p[3] = static_cast<std::uint8_t>((v >> 24) & 0xffu);
}

inline std::uint32_t read_u32_le(const std::uint8_t* p) noexcept {
  return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
         (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

inline void append_u32_le(std::vector<std::uint8_t>& out, std::uint32_t v) {
  std::uint8_t tmp[4];
  write_u32_le(tmp, v);
  out.insert(out.end(), tmp, tmp + 4);
}

}  // namespace laserdesk::rtc::rif
