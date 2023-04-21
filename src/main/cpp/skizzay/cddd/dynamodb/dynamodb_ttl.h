#pragma once

#include <chrono>
#include <string>

namespace skizzay::cddd::dynamodb {
struct time_to_live {
  std::string name;
  std::chrono::utc_seconds value;
};
} // namespace skizzay::cddd::dynamodb
