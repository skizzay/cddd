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
        struct write_fn final {
            template<typename T, std::size_t N>
                requires requires(T &sink, std::span<std::byte const, N> const bytes) {
                    { sink.write(bytes) } -> std::convertible_to<std::size_t>;
                }
            constexpr std::size_t operator()(T &sink, std::span<std::byte const> const bytes
            ) const noexcept(noexcept(sink.write(bytes))) {
                return sink.write(bytes);
            }

            template<typename T, typename Ch, typename Tr>
                requires requires(T &sink, std::basic_string_view<Ch, Tr> const s) {
                    { sink.write(s) } -> std::convertible_to<std::size_t>;
                }
            constexpr std::size_t operator()(T &sink, std::basic_string_view<Ch, Tr> const s
            ) const noexcept(noexcept(sink.write(s))) {
                return sink.write(s);
            }
        };

        struct seek_write_fn final {
            template<typename T, std::signed_integral Offset>
                requires requires(T &sink, Offset const p, seek_origin const origin) {
                    { sink.seek_write(p, origin) };
                }
            constexpr decltype(auto) operator()(T &sink, Offset const p, seek_origin const origin
            ) const noexcept(noexcept(sink.seek_write(p, origin))) {
                return sink.seek_write(p, origin);
            }

            template<typename T, std::signed_integral Offset>
            constexpr decltype(auto) operator()(T &sink, Offset const p, std::ios_base::seekdir const seekdir
            ) const noexcept(noexcept((*this)(sink, p, to_seek_origin(seekdir)))) {
                return (*this)(sink, p, to_seek_origin(seekdir));
            }
        };

        struct flush_fn final {
            template<typename T>
                requires requires(T &sink) {
                    { sink.flush() };
                }
            constexpr decltype(auto) operator()(T &sink) const noexcept(noexcept(sink.flush())) {
                return sink.flush();
            }
        };
    }

    inline namespace sink_cpo {
        constexpr inline sink_details::write_fn write{};
        constexpr inline sink_details::seek_write_fn seek_write{};
        constexpr inline sink_details::flush_fn flush{};
    }

    template<typename T>
    concept sink = std::is_invocable_r_v<std::size_t, decltype(write), T &, std::span<std::byte const> const>
                   && std::is_invocable_r_v<std::size_t, decltype(write), T &, std::string_view const>
                   && std::is_invocable_r_v<std::size_t, decltype(write), T &, std::wstring_view const>
                   && std::is_invocable_v<decltype(seek_write), T &, std::streamoff, seek_origin>
                   && std::is_invocable_v<decltype(flush), T &>;
}

#endif //SINK_H
