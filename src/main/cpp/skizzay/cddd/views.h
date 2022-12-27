#pragma once

#include <algorithm>
#include <range/v3/view/chunk.hpp>
#include <range/v3/view/concat.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/single.hpp>
#include <range/v3/view/transform.hpp>
#include <ranges>

namespace skizzay::cddd::views {
inline constexpr auto all = std::ranges::views::all;
#ifdef __cpp_lib_ranges_chunk
inline constexpr auto chunk = std::ranges::views::chunk;
#else
inline constexpr auto chunk = ranges::views::chunk;
#endif
inline constexpr auto concat = ranges::views::concat;
// TODO: enumerate = zip_view<itoa<weakly_incrementable>, input_range> using an
// adaptor closure
inline constexpr auto enumerate = ranges::views::enumerate;
inline constexpr auto single = ranges::views::single;
inline constexpr auto transform = ranges::views::transform;
} // namespace skizzay::cddd::views
