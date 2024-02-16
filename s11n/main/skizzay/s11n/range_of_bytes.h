//
// Created by andrew on 2/9/24.
//

#pragma once

#include <type_traits>
#include <ranges>

namespace skizzay::s11n {
    template<typename R>
    concept range_of_bytes = std::ranges::contiguous_range<R>
                             && std::is_standard_layout_v<std::ranges::range_value_t<R> >
                             && std::is_trivial_v<std::ranges::range_value_t<R> >
                             && (!(std::is_pointer_v<std::ranges::range_value_t<R> >
                                   || std::is_member_pointer_v<std::ranges::range_value_t<R> >));

    static_assert(range_of_bytes<std::span<std::byte const> >);
}