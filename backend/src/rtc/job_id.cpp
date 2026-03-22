#include "job_id.hpp"

#include <random>

namespace laserdesk::rtc {

std::string make_demo_job_id() {
  static thread_local std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution<unsigned> d(0, 15);
  const char* hex = "0123456789abcdef";
  auto nib = [&]() { return hex[d(gen)]; };
  std::string u;
  u.reserve(36);
  for (int i = 0; i < 8; ++i) u += nib();
  u += '-';
  for (int i = 0; i < 4; ++i) u += nib();
  u += "-4";
  for (int i = 0; i < 3; ++i) u += nib();
  u += '-';
  u += nib();
  for (int i = 0; i < 3; ++i) u += nib();
  u += '-';
  for (int i = 0; i < 12; ++i) u += nib();
  return u;
}

}  // namespace laserdesk::rtc
