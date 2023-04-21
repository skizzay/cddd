#pragma once

#include <chrono>
#include <type_traits>

namespace skizzay::cddd::dynamodb {
namespace clock_details_ {

template <typename> inline constexpr bool is_complete_type_v = false;

template <typename T>
requires(0 < sizeof(T)) inline constexpr bool is_complete_type_v<T> = true;
} // namespace clock_details_

using default_clock_t = std::conditional_t<
    clock_details_::is_complete_type_v<std::chrono::utc_clock>,
    std::chrono::utc_clock, std::chrono::system_clock>;

using default_timestamp_t = std::conditional_t<
    clock_details_::is_complete_type_v<std::chrono::utc_clock>,
    std::chrono::utc_seconds, std::chrono::sys_seconds>;
} // namespace skizzay::cddd::dynamodb
