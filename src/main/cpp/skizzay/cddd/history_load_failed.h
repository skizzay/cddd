#pragma once

#include <stdexcept>

namespace skizzay::cddd {
struct history_load_failed : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct event_deserialization_failed : history_load_failed {
  using history_load_failed::history_load_failed;
};
} // namespace skizzay::cddd