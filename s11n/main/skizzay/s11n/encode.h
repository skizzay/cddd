//
// Created by andrew on 1/25/24.
//

#ifndef ENCODE_H
#define ENCODE_H

#include <algorithm>
#include <array>
#include <bit>
#if __has_include(<ieee754.h>)
#include <ieee754.h>
#endif
#include <concepts>

// ReSharper disable once CppInconsistentNaming
namespace skizzay::s11n {

    constexpr inline std::endian float_word_order = []() -> std::endian {
        if constexpr (std::endian::big == std::endian::native) {
            return std::endian::big;
        }
        else {
            // I've spent too much time on this, and I'm not sure if it's even possible to detect the float word order
            // without using preprocessor macros.
            return __FLOAT_WORD_ORDER == __ORDER_BIG_ENDIAN__ ? std::endian::big : std::endian::little;
        }
    }();

namespace encode_details {

template<std::endian E>
struct encode_fn final {
    template<std::integral T>
    constexpr std::array<std::byte, sizeof(T)> operator()(T const value) const noexcept {
        auto result = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
        if constexpr (E != std::endian::native) {
            std::ranges::reverse(result);
        }
        return result;
    }

    constexpr std::array<std::byte, sizeof(float)> operator()(float const value) const noexcept {
        auto result = std::bit_cast<std::array<std::byte, sizeof(float)>>(value);
        if constexpr (E != std::endian::native) {
            std::ranges::reverse(result);
        }
        return result;
    }

    constexpr std::array<std::byte, sizeof(double)> operator()(double const value) const noexcept {
        auto parts = std::bit_cast<std::array<std::array<std::byte, sizeof(double) / 2>, 2>>(value);
        // TODO: Implement this correctly
        if constexpr (std::endian::big == std::endian::native || std::endian::big == float_word_order) {
            if constexpr (std::endian::big == E) {
                std::ranges::reverse(parts[0]);
                std::ranges::reverse(parts[1]);
            }
            else {
                std::ranges::reverse(parts[1]);
                std::ranges::reverse(parts[0]);
            }
        }
        else {
            if constexpr (std::endian::big == E) {
                std::ranges::reverse(parts[1]);
                std::ranges::reverse(parts[0]);
            }
            else {
                std::ranges::reverse(parts[0]);
                std::ranges::reverse(parts[1]);
            }
        }
        return std::bit_cast<std::array<std::byte, sizeof(double)>>(parts);
    }

    constexpr std::array<std::byte, sizeof(long double)> operator()(long double const value) const noexcept {
        auto parts = std::bit_cast<std::array<std::byte, sizeof(long double)>>(value);
        // TODO: Implement this correctly
        if constexpr (E != std::endian::native) {
            std::ranges::reverse(parts);
        }
        return parts;
    }
};

}

inline namespace encode_cpo {
inline constexpr encode_details::encode_fn<std::endian::big> encode_big{};
inline constexpr encode_details::encode_fn<std::endian::little> encode_little{};
inline constexpr encode_details::encode_fn<std::endian::native> encode_native{};
template<std::endian E>
inline constexpr encode_details::encode_fn<E> encode{};
}

} // skizzay

#endif //ENCODE_H
