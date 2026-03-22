#include "telegram_raw.hpp"

#include "byte_order.hpp"

#include <sstream>

namespace laserdesk::rtc::rif {

std::vector<std::uint8_t> build_command_telegram(std::uint32_t seq, std::uint32_t format,
                                                 const std::vector<std::uint32_t>& cmd_payload_words) {
  const std::uint32_t pl_bytes =
      static_cast<std::uint32_t>(cmd_payload_words.size() * sizeof(std::uint32_t));
  std::vector<std::uint8_t> out;
  out.reserve(kTgmHeaderBytes + pl_bytes);
  std::uint8_t hdr[20];
  write_u32_le(hdr + 0, pl_bytes);
  write_u32_le(hdr + 4, kTgmVersion01000000);
  write_u32_le(hdr + 8, seq);
  write_u32_le(hdr + 12, kTypeCommand);
  write_u32_le(hdr + 16, format);
  out.insert(out.end(), hdr, hdr + sizeof(hdr));
  for (std::uint32_t w : cmd_payload_words) {
    append_u32_le(out, w);
  }
  return out;
}

ParsedAnswer parse_answer_telegram(const std::uint8_t* data, std::size_t len,
                                   std::uint32_t expect_seq, std::uint32_t expect_format) {
  ParsedAnswer r;
  if (len < kTgmHeaderBytes + 4) {
    r.parse_error = "Answer too short";
    return r;
  }
  const std::uint32_t pl_len = read_u32_le(data + 0);
  const std::uint32_t ver = read_u32_le(data + 4);
  r.seqnum = read_u32_le(data + 8);
  const std::uint32_t typ = read_u32_le(data + 12);
  const std::uint32_t fmt = read_u32_le(data + 16);

  if (ver != kTgmVersion01000000) {
    r.parse_error = "Unexpected telegram version in answer";
    return r;
  }
  if (typ != kTypeAnswer) {
    r.parse_error = "Not an answer telegram";
    return r;
  }
  if (fmt != expect_format) {
    r.parse_error = "Answer format mismatch";
    return r;
  }
  if (r.seqnum != expect_seq) {
    r.parse_error = "Answer seqnum mismatch";
    return r;
  }
  if (pl_len + kTgmHeaderBytes > len) {
    r.parse_error = "Answer length field exceeds datagram";
    return r;
  }
  if (pl_len % 4 != 0) {
    r.parse_error = "Answer payload not multiple of 4";
    return r;
  }
  const std::size_t n_words = pl_len / 4;
  r.pl_words.resize(n_words);
  for (std::size_t i = 0; i < n_words; ++i) {
    r.pl_words[i] = read_u32_le(data + kTgmHeaderBytes + i * 4);
  }
  if (!r.pl_words.empty()) {
    r.last_error = r.pl_words[0];
  }
  r.ok = true;
  return r;
}

std::string describe_last_error(std::uint32_t last_error) {
  if (last_error_invalid_telegram(last_error)) {
    return "invalid Command Telegram (LastError bit 31)";
  }
  std::ostringstream o;
  o << "LastError=0x" << std::hex << last_error;
  if (last_error & (1u << 0)) o << " ERROR_TGM_SIZE";
  if (last_error & (1u << 1)) o << " ERROR_HEADER_VERSION";
  if (last_error & (1u << 2)) o << " ERROR_HEADER_LENGTH";
  if (last_error & (1u << 3)) o << " ERROR_HEADER_TYPE";
  if (last_error & (1u << 4)) o << " ERROR_HEADER_FORMAT";
  if (last_error & (1u << 5)) o << " ERROR_PAYLOAD_UNKNOWN_CMD_ID";
  if (last_error & (1u << 6)) o << " ERROR_PAYLOAD_INVALID";
  return o.str();
}

bool last_error_invalid_telegram(std::uint32_t last_error) {
  return (last_error >> 31) != 0u;
}

}  // namespace laserdesk::rtc::rif
