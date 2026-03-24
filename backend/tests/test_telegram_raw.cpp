#include "rtc/rif/byte_order.hpp"
#include "rtc/rif/telegram_raw.hpp"

#include <cstring>

#include <gtest/gtest.h>
#include <vector>

using laserdesk::rtc::rif::build_command_telegram;
using laserdesk::rtc::rif::kRdcGetHeadPara;
using laserdesk::rtc::rif::kRdcGetStatus;
using laserdesk::rtc::rif::kTgmHeaderBytes;
using laserdesk::rtc::rif::kTgmVersion01000000;
using laserdesk::rtc::rif::kTypeAnswer;
using laserdesk::rtc::rif::kTypeCommand;
using laserdesk::rtc::rif::ParsedAnswer;
using laserdesk::rtc::rif::answer_raw_matches_seq_and_format;
using laserdesk::rtc::rif::parse_answer_telegram;
using laserdesk::rtc::rif::try_parse_seq_sync_answer;
using laserdesk::rtc::rif::read_u32_le;
using laserdesk::rtc::rif::write_u32_le;

TEST(TelegramRaw, BuildGetHeadPara) {
  auto pkt = build_command_telegram(3u, 1u, {kRdcGetHeadPara, 1u, 1u});
  ASSERT_GE(pkt.size(), kTgmHeaderBytes + 12u);
  EXPECT_EQ(read_u32_le(pkt.data() + kTgmHeaderBytes + 0), kRdcGetHeadPara);
  EXPECT_EQ(read_u32_le(pkt.data() + kTgmHeaderBytes + 4), 1u);
  EXPECT_EQ(read_u32_le(pkt.data() + kTgmHeaderBytes + 8), 1u);
}

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

TEST(TelegramRaw, ParseSyntheticAnswerGetHeadParaDouble) {
  std::vector<std::uint8_t> pkt(kTgmHeaderBytes + 16u);
  write_u32_le(pkt.data() + 0, 16u);
  write_u32_le(pkt.data() + 4, kTgmVersion01000000);
  write_u32_le(pkt.data() + 8, 9u);
  write_u32_le(pkt.data() + 12, kTypeAnswer);
  write_u32_le(pkt.data() + 16, 1u);
  write_u32_le(pkt.data() + 20, 0u);
  write_u32_le(pkt.data() + 24, kRdcGetHeadPara);
  write_u32_le(pkt.data() + 28, 0u);
  write_u32_le(pkt.data() + 32, 0x40600000u);

  auto a = parse_answer_telegram(pkt.data(), pkt.size(), 9u, 1u);
  ASSERT_TRUE(a.ok);
  ASSERT_GE(a.pl_words.size(), 4u);
  EXPECT_EQ(a.pl_words[1], kRdcGetHeadPara);
  double d = 0.0;
  std::uint32_t parts[2] = {a.pl_words[2], a.pl_words[3]};
  std::memcpy(&d, parts, sizeof(d));
  EXPECT_DOUBLE_EQ(d, 128.0);
}

TEST(TelegramRaw, AnswerRawMatchesSeqAndFormat) {
  std::vector<std::uint8_t> pkt(36);
  write_u32_le(pkt.data() + 0, 16u);
  write_u32_le(pkt.data() + 4, kTgmVersion01000000);
  write_u32_le(pkt.data() + 8, 7u);
  write_u32_le(pkt.data() + 12, kTypeAnswer);
  write_u32_le(pkt.data() + 16, 0u);
  EXPECT_TRUE(answer_raw_matches_seq_and_format(pkt.data(), pkt.size(), 7u, 0u));
  EXPECT_FALSE(answer_raw_matches_seq_and_format(pkt.data(), pkt.size(), 8u, 0u));
  EXPECT_FALSE(answer_raw_matches_seq_and_format(pkt.data(), pkt.size(), 7u, 1u));
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

TEST(TelegramRaw, ParseSeqSyncAnswerMatchesWrapper) {
  // rtc6_rif_wrapper.cpp: seqnum = answ.payload.buffer[0] + 1;
  ParsedAnswer a;
  a.ok = true;
  a.last_error = 5000u;
  a.pl_words = {5000u};
  std::uint32_t last = 0;
  ASSERT_TRUE(try_parse_seq_sync_answer(a, last));
  EXPECT_EQ(last, 5000u);
}
