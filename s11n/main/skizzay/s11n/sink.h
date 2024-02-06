//
// Created by andrew on 1/25/24.
//

#ifndef SINK_H
#define SINK_H

#include <concepts>
#include <cstddef>
#include <ios>
#include <span>
#include <string_view>

#include "seek_origin.h"

// ReSharper disable once CppInconsistentNaming
// ReSharper disable once CppEnforceNestedNamespacesStyle
namespace skizzay::s11n { namespace sink_details {
        struct sink_write_fn final {
            constexpr std::size_t operator()(auto &sink, auto const &payload
            ) const noexcept(noexcept(sink.write(payload))) {
                return sink.write(payload);
            }

            template<typename T, typename Ch, std::size_t N>
                requires std::same_as<Ch, char> || std::same_as<Ch, wchar_t> || std::same_as<Ch, char8_t> ||
                         std::same_as<Ch, char16_t> || std::same_as<Ch, char32_t>
            constexpr std::size_t operator()(T &sink, Ch const (&str)[N]
            ) const noexcept(noexcept((*this)(sink, std::basic_string_view<Ch>{str}))) {
                return (*this)(sink, std::basic_string_view<Ch>{str});
            }
        };

        struct seek_write_fn final {
            template<typename T, std::signed_integral Offset>
                requires requires(T &sink, Offset const p, seek_origin const origin) {
                    { sink.sink_seek(p, origin) };
                }
            constexpr decltype(auto) operator()(T &sink, Offset const p, seek_origin const origin
            ) const noexcept(noexcept(sink.sink_seek(p, origin))) {
                return sink.sink_seek(p, origin);
            }

            template<typename T, std::signed_integral Offset>
            constexpr decltype(auto) operator()(T &sink, Offset const p, std::ios_base::seekdir const seekdir
            ) const noexcept(noexcept((*this)(sink, p, to_seek_origin(seekdir)))) {
                return (*this)(sink, p, to_seek_origin(seekdir));
            }
        };

        struct sink_flush_fn final {
            template<typename T>
                requires requires(T &sink) {
                    { sink.flush() };
                }
            constexpr decltype(auto) operator()(T &sink) const noexcept(noexcept(sink.flush())) {
                return sink.flush();
            }
        };

        void ensure_size(auto &, std::size_t) = delete;

        struct sink_ensure_size_fn final {
            constexpr void operator()(auto &, std::size_t const) const noexcept {
            }

            template<typename T>
            requires requires(T &sink, std::size_t const n) {
                { ensure_size(sink, n) };
            }
            constexpr decltype(auto) operator()(T &sink, std::size_t const n) const noexcept(noexcept(ensure_size(sink, n))) {
                return ensure_size(sink, n);
            }

            template<typename T>
            requires requires(T &sink, std::size_t const n) {
                { sink.ensure_size(n) };
            }
            constexpr decltype(auto) operator()(T &sink, std::size_t const n) const noexcept(noexcept(sink.ensure_growth(n))) {
                return sink.ensure_size(n);
            }
        };

        struct sink_position_fn final {
            template<typename T>
                requires requires(T const &sink) {
                    { sink.sink_position() } -> std::signed_integral;
                }
            constexpr std::signed_integral auto operator(
            )(T const &sink) const noexcept(noexcept(sink.sink_position())) {
                return sink.sink_position();
            }
        };
    }

    inline namespace sink_cpo {
        constexpr inline sink_details::sink_write_fn sink_write{};
        constexpr inline sink_details::seek_write_fn sink_seek{};
        constexpr inline sink_details::sink_flush_fn sink_flush{};
        constexpr inline sink_details::sink_position_fn sink_position{};
        constexpr inline sink_details::sink_ensure_size_fn sink_ensure_size{};
    }

    template<typename T>
    concept sink = std::is_invocable_v<decltype(sink_write), T &, std::span<std::byte const> const>
                   && std::is_invocable_v<decltype(sink_write), T &, std::string_view const>
                   && std::is_invocable_v<decltype(sink_write), T &, std::wstring_view const>
                   && std::is_invocable_v<decltype(sink_write), T &, char const>
                   && std::is_invocable_v<decltype(sink_seek), T &, std::streamoff, seek_origin>
                   && std::is_invocable_v<decltype(sink_flush), T &>
                   && std::is_invocable_v<decltype(sink_position), T const &>;
}

#endif //SINK_H
