#include "rtc/rif/byte_order.hpp"
#include "rtc/rif/telegram_raw.hpp"

#include <gtest/gtest.h>
#include <vector>

using laserdesk::rtc::rif::build_command_telegram;
using laserdesk::rtc::rif::kRdcGetStatus;
using laserdesk::rtc::rif::kTgmHeaderBytes;
using laserdesk::rtc::rif::kTgmVersion01000000;
using laserdesk::rtc::rif::kTypeAnswer;
using laserdesk::rtc::rif::kTypeCommand;
using laserdesk::rtc::rif::parse_answer_telegram;
using laserdesk::rtc::rif::read_u32_le;
using laserdesk::rtc::rif::write_u32_le;

TEST(TelegramRaw, BuildGetStatus) {
  auto pkt = build_command_telegram(7u, 0u, {kRdcGetStatus});
  ASSERT_EQ(pkt.size(), kTgmHeaderBytes + 4u);
  EXPECT_EQ(read_u32_le(pkt.data() + 0), 4u);
  EXPECT_EQ(read_u32_le(pkt.data() + 4), kTgmVersion01000000);
  EXPECT_EQ(read_u32_le(pkt.data() + 8), 7u);
  EXPECT_EQ(read_u32_le(pkt.data() + 12), kTypeCommand);
  EXPECT_EQ(read_u32_le(pkt.data() + 16), 0u);
  EXPECT_EQ(read_u32_le(pkt.data() + 20), kRdcGetStatus);
}

TEST(TelegramRaw, ParseSyntheticAnswerGetStatus) {
  std::vector<std::uint8_t> pkt(36);
  write_u32_le(pkt.data() + 0, 16u);
  write_u32_le(pkt.data() + 4, kTgmVersion01000000);
  write_u32_le(pkt.data() + 8, 7u);
  write_u32_le(pkt.data() + 12, kTypeAnswer);
  write_u32_le(pkt.data() + 16, 0u);
  write_u32_le(pkt.data() + 20, 0u);
  write_u32_le(pkt.data() + 24, kRdcGetStatus);
  write_u32_le(pkt.data() + 28, 42u);
  write_u32_le(pkt.data() + 32, 99u);

  auto a = parse_answer_telegram(pkt.data(), pkt.size(), 7u, 0u);
  ASSERT_TRUE(a.ok);
  ASSERT_GE(a.pl_words.size(), 4u);
  EXPECT_EQ(a.pl_words[0], 0u);
  EXPECT_EQ(a.pl_words[1], kRdcGetStatus);
  EXPECT_EQ(a.pl_words[2], 42u);
  EXPECT_EQ(a.pl_words[3], 99u);
}
