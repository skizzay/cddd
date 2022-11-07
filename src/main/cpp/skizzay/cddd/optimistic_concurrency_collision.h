#pragma once

#include "skizzay/cddd/commit_failed.h"

namespace skizzay::cddd {
struct optimistic_concurrency_collision : commit_failed {
  explicit optimistic_concurrency_collision(std::string const &message,
                                            std::size_t const version_found,
                                            std::size_t const version_expected)
      : commit_failed{message}, version_found_{version_found},
        version_expected_{version_expected} {}

  std::size_t version_found() const noexcept { return version_found_; }

  std::size_t version_expected() const noexcept { return version_expected_; }

private:
  std::size_t version_found_;
  std::size_t version_expected_;
};
} // namespace skizzay::cddd
