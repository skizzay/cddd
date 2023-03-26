#pragma once

#include "skizzay/cddd/identifier.h"

#include <stdexcept>
#include <type_traits>
#include <utility>

namespace skizzay::cddd {

template <concepts::identifier Key>
requires std::is_same_v<Key, std::remove_cvref_t<Key>>
struct key_not_found : std::invalid_argument {
  key_not_found(Key key_value)
      : std::invalid_argument{"Key not found"}, key_{std::move(key_value)} {}

  constexpr Key const &key() const noexcept { return key_; }

private:
  Key key_;
};

inline constexpr auto throw_key_not_found =
    [] [[noreturn]] (concepts::identifier auto key) {
  throw key_not_found{std::move(key)};
};

} // namespace skizzay::cddd