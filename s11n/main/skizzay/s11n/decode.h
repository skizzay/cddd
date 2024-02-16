//
// Created by andrew on 1/26/24.
//

#ifndef DECODE_H
#define DECODE_H

#include <algorithm>
#include <bit>
#include <span>

// ReSharper disable once CppInconsistentNaming
namespace skizzay::s11n {

    namespace decode_details {
        template<std::endian E>
        struct decode_fn final {
            template<std::size_t N, std::integral I>
            requires (sizeof(I) <= N)
            constexpr std::size_t operator()(std::span<std::byte const, N> const bytes, I &out) const noexcept {
                if (bytes.size() < sizeof(I)) {
                    return 0;
                }
                std::span<std::byte, sizeof(I)> const bytes_to_decode{static_cast<std::byte *>(&out), sizeof(I)};
                if constexpr (E != std::endian::native) {
                    std::ranges::reverse(bytes_to_decode);
                }
                return sizeof(I);
            }

            template<std::size_t N, std::integral I>
            requires (sizeof(I) <= N)
            constexpr std::size_t operator()(std::array<std::byte, N> bytes_to_decode, I &out) const noexcept {
                if constexpr (E != std::endian::native) {
                    std::ranges::reverse(bytes_to_decode);
                }
                out = std::bit_cast<I>(bytes_to_decode);
                return sizeof(I);
            }
        };
    }

    inline namespace decode_cpo {
        constexpr inline decode_details::decode_fn<std::endian::big> decode_big{};
        constexpr inline decode_details::decode_fn<std::endian::little> decode_little{};
        constexpr inline decode_details::decode_fn<std::endian::native> decode_native{};
        template<std::endian E>
        constexpr inline decode_details::decode_fn<E> decode{};
    }
}

#endif //DECODE_H
