#pragma once

#include <stdexcept>

namespace skizzay::cddd {
struct commit_failed : std::runtime_error {
  using std::runtime_error::runtime_error;
};
} // namespace skizzay::cddd
