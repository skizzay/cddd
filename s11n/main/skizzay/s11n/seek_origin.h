//
// Created by andrew on 1/25/24.
//

#ifndef SEEK_ORIGIN_H
#define SEEK_ORIGIN_H

#include <cstdio>
#include <ios>
#include <utility>

// ReSharper disable once CppInconsistentNaming
namespace skizzay::s11n {
    enum class seek_origin {
        beginning = SEEK_SET,
        current = SEEK_CUR,
        end = SEEK_END
    };

    constexpr inline auto to_seekdir = [](seek_origin const origin) noexcept {
        switch (origin) {
            case seek_origin::beginning:
                return std::ios_base::beg;
            case seek_origin::current:
                return std::ios_base::cur;
            case seek_origin::end:
                return std::ios_base::end;
            default:
                std::unreachable();
        }
    };

    constexpr inline auto to_seek_origin = [](std::ios_base::seekdir const origin) noexcept {
        switch (origin) {
            case std::ios_base::beg:
                return seek_origin::beginning;
            case std::ios_base::cur:
                return seek_origin::current;
            case std::ios_base::end:
                return seek_origin::end;
            default:
                std::unreachable();
        }
    };
}

#endif //SEEK_ORIGIN_H
