//
// Created by andrew on 1/26/24.
//

#ifndef SOURCE_H
#define SOURCE_H

#include "seek_origin.h"

#include <concepts>
#include <span>
#include <vector>

// ReSharper disable once CppInconsistentNaming
namespace skizzay::s11n { namespace source_details {
        template<typename R>
        concept buffer = std::ranges::contiguous_range<R> && std::ranges::sized_range<R>;

        struct source_read_fn final {
            template<typename T, std::ranges::contiguous_range B>
                requires std::ranges::sized_range<B> && requires(source_read_fn const f, T &source, B &buffer) {
                    { f(source, buffer, std::ranges::size(buffer)) } -> std::unsigned_integral;
                }
            constexpr std::unsigned_integral auto operator()(T &source, B &buffer
            ) const noexcept(
                noexcept((*this)(source, buffer, std::ranges::size(buffer)))) {
                return (*this)(source, buffer, std::ranges::size(buffer));
            }

            template<typename T, std::ranges::contiguous_range B, std::unsigned_integral I>
                requires requires(T &source, B &buffer, I const n) {
                    { source.read(buffer, n) } -> std::unsigned_integral;
                }
            constexpr std::unsigned_integral auto operator()(T &source, B &buffer,
                                                             I const n
            ) const noexcept(
                noexcept(source.read(buffer, n))) {
                return source.read(buffer, n);
            }
        };

        struct source_seek_fn final {
            template<typename T, std::signed_integral Offset>
                requires requires(T &source, Offset const p, seek_origin const origin) {
                    { source.source_seek(p, origin) };
                }
            constexpr decltype(auto) operator()(T &source, Offset const p, seek_origin const origin
            ) const noexcept(noexcept(source.source_seek(p, origin))) {
                return source.source_seek(p, origin);
            }
        };

        struct source_position_fn final {
            template<typename T>
                requires requires(T const &t) {
                    { t.source_position() } -> std::signed_integral;
                }
            constexpr std::signed_integral auto operator()(T const &t) const noexcept(noexcept(t.source_position())) {
                return t.source_position();
            }
        };
    }

    inline namespace source_cpo {
        constexpr inline source_details::source_read_fn source_read{};
        constexpr inline source_details::source_seek_fn source_seek{};
        constexpr inline source_details::source_position_fn source_position{};
    }

    template<typename T>
    concept source = std::is_invocable_v<decltype(source_read), T &, std::span<std::byte> &, std::span<std::byte>::size_type>
                     && std::is_invocable_v<decltype(source_read), T &, std::vector<std::byte> &, std::vector<std::byte>::size_type>
                     && std::is_invocable_v<decltype(source_seek), T &, std::ptrdiff_t, seek_origin const>
                     && std::is_invocable_v<decltype(source_position), T const &>;
}

#endif //SOURCE_H
