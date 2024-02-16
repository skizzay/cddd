//
// Created by andrew on 2/13/24.
//

#pragma once

#include <concepts>
#include <cstdint>
#include "boolean.h"
#include "range_of_bytes.h"
#include "seek_origin.h"

namespace skizzay::s11n { namespace io_device_details {
        bool is_open(auto const &) noexcept = delete;

        struct is_open_fn final {
            template<typename T>
                requires requires(T const &t) {
                    { t.is_open() } -> boolean;
                }
            constexpr bool operator()(T const &t) const noexcept(noexcept(t.is_open())) {
                return t.is_open();
            }

            template<typename T>
                requires requires(T const &t) {
                    { t.isOpen() } -> boolean;
                }
            constexpr bool operator()(T const &t) const noexcept(noexcept(t.isOpen())) {
                return t.isOpen();
            }

            template<typename T>
                requires requires(T const &t) {
                    { is_open(t) } -> boolean;
                }
            constexpr bool operator()(T const &t) const noexcept(noexcept(is_open(t))) {
                return is_open(t);
            }
        };

        void close(auto &) = delete;

        struct close_fn final {
            template<typename T>
                requires requires(T &t) {
                    { t.close() };
                }
            constexpr decltype(auto) operator()(T &t) const noexcept(noexcept(t.close())) {
                return t.close();
            }

            template<typename T>
                requires requires(T &t) {
                    { close(t) };
                }
            constexpr decltype(auto) operator()(T &t) const noexcept(noexcept(close(t))) {
                return close(t);
            }
        };

        bool at_eof(auto const &) noexcept = delete;

        bool eof(auto const &) noexcept = delete;

        struct at_eof_fn final {
            template<typename T>
                requires requires(T const &t) {
                    { t.at_eof() } -> boolean;
                }
            constexpr bool operator()(T const &t) const noexcept(noexcept(t.at_eof())) {
                return t.at_eof();
            }

            template<typename T>
                requires requires(T const &t) {
                    { t.atEof() } -> boolean;
                }
            constexpr bool operator()(T const &t) const noexcept(noexcept(t.atEof())) {
                return t.atEof();
            }

            template<typename T>
                requires requires(T const &t) {
                    { at_eof(t) } -> boolean;
                }
            constexpr bool operator()(T const &t) const noexcept(noexcept(at_eof(t))) {
                return at_eof(t);
            }

            template<typename T>
                requires requires(T const &t) {
                    { eof(t) } -> boolean;
                }
            constexpr bool operator()(T const &t) const noexcept(noexcept(eof(t))) {
                return eof(t);
            }

            template<typename T>
                requires requires(T const &t) {
                    { t.eof() } -> boolean;
                }
            constexpr bool operator()(T const &t) const noexcept(noexcept(t.eof())) {
                return t.eof();
            }
        };

        int write_position(auto const &) noexcept = delete;

        int writePosition(auto const &) noexcept = delete;

        struct write_position_fn final {
            template<typename T>
                requires requires(T const &t) {
                    { t.write_position() } -> std::signed_integral;
                }
            constexpr std::signed_integral auto operator()(T const &t) const noexcept {
                return t.write_position();
            }

            template<typename T>
                requires requires(T const &t) {
                    { write_position(t) } -> std::signed_integral;
                }
            constexpr std::signed_integral auto operator()(T const &t) const noexcept {
                return write_position(t);
            }

            template<typename T>
                requires requires(T const &t) {
                    { t.writePosition() } -> std::signed_integral;
                }
            constexpr std::signed_integral auto operator()(T const &t) const noexcept {
                return t.writePosition();
            }

            template<typename T>
                requires requires(T const &t) {
                    { writePosition(t) } -> std::signed_integral;
                }
            constexpr std::signed_integral auto operator()(T const &t) const noexcept {
                return writePosition(t);
            }
        };

        int read_position(auto const &) noexcept = delete;

        int readPosition(auto const &) noexcept = delete;

        struct read_position_fn final {
            template<typename T>
                requires requires(T const &t) {
                    { t.read_position() } -> std::signed_integral;
                }
            constexpr std::signed_integral auto operator()(T const &t) const noexcept {
                return t.read_position();
            }

            template<typename T>
                requires requires(T const &t) {
                    { read_position(t) } -> std::signed_integral;
                }
            constexpr std::signed_integral auto operator()(T const &t) const noexcept {
                return read_position(t);
            }

            template<typename T>
                requires requires(T const &t) {
                    { t.readPosition() } -> std::signed_integral;
                }
            constexpr std::signed_integral auto operator()(T const &t) const noexcept {
                return t.readPosition();
            }

            template<typename T>
                requires requires(T const &t) {
                    { readPosition(t) } -> std::signed_integral;
                }
            constexpr std::signed_integral auto operator()(T const &t) const noexcept {
                return readPosition(t);
            }
        };

        std::size_t read_remaining(auto const &) noexcept = delete;

        std::size_t readRemaining(auto const &) noexcept = delete;

        struct read_remaining_fn final {
            template<typename T>
                requires requires(T const &t) {
                    { t.read_remaining() } -> std::unsigned_integral;
                }
            constexpr std::unsigned_integral auto operator()(T const &t) const noexcept {
                return t.read_remaining();
            }

            template<typename T>
                requires requires(T const &t) {
                    { t.readRemaining() } -> std::unsigned_integral;
                }
            constexpr std::unsigned_integral auto operator()(T const &t) const noexcept {
                return t.readRemaining();
            }

            template<typename T>
                requires requires(T const &t) {
                    { read_remaining(t) } -> std::unsigned_integral;
                }
            constexpr std::unsigned_integral auto operator()(T const &t) const noexcept {
                return read_remaining(t);
            }

            template<typename T>
                requires requires(T const &t) {
                    { readRemaining(t) } -> std::unsigned_integral;
                }
            constexpr std::unsigned_integral auto operator()(T const &t) const noexcept {
                return readRemaining(t);
            }

            template<typename T>
                requires std::invocable<read_position_fn const, T const &>
                         && std::invocable<write_position_fn const, T const &>
                         && (!(requires(T const &t) {
                                   { t.read_remaining() } -> std::unsigned_integral;
                               } || requires(T const &t) {
                                   { t.readRemaining() } -> std::unsigned_integral;
                               } || requires(T const &t) {
                                   { read_remaining(t) } -> std::unsigned_integral;
                               } || requires(T const &t) {
                                   { readRemaining(t) } -> std::unsigned_integral;
                               }))
            constexpr std::unsigned_integral auto operator()(T const &t) const noexcept {
                constexpr write_position_fn write_position{};
                constexpr read_position_fn read_position{};
                auto const result = write_position(t) - read_position(t);
                return static_cast<std::make_unsigned_t<decltype(result)>>(result);
            }
        };

        std::int64_t size(auto const &) noexcept = delete;

        struct size_fn final {
            template<typename T>
                requires requires(T const &t) {
                    { t.size() } -> std::signed_integral;
                }
            constexpr std::signed_integral auto operator()(T const &t) const noexcept {
                return t.size();
            }

            template<typename T>
                requires requires(T const &t) {
                    { size(t) } -> std::signed_integral;
                }
            constexpr std::signed_integral auto operator()(T const &t) const noexcept {
                return size(t);
            }
        };

        void truncate(auto &, std::signed_integral auto) = delete;

        struct truncate_fn final {
            template<typename T, std::signed_integral I>
                requires requires(T &t, I const n) {
                    { t.truncate(n) };
                }
            constexpr decltype(auto) operator()(T &t, I const n) const {
                return t.truncate(n);
            }

            template<typename T, std::signed_integral I>
                requires requires(T &t, I const n) {
                    { truncate(t, n) };
                }
            constexpr decltype(auto) operator()(T &t, I const n) const {
                return truncate(t, n);
            }
        };

        void reserve(auto &, std::unsigned_integral auto) = delete;

        struct reserve_fn final {
            template<typename T, std::unsigned_integral U>
                requires requires(T &t, U const n) {
                    { t.reserve(n) };
                }
            constexpr decltype(auto) operator()(T &t, U const n) const {
                return t.reserve(n);
            }

            template<typename T, std::unsigned_integral U>
                requires requires(T &t, U const n) {
                    { reserve(t, n) };
                }
            constexpr decltype(auto) operator()(T &t, U const n) const {
                return reserve(t, n);
            }
        };

        void seek_write(auto &, std::signed_integral auto, seek_origin) = delete;

        struct seek_write_fn final {
            template<typename T, std::signed_integral I>
                requires requires(T &t, I const p, seek_origin const o) {
                    { t.seek_write(p, o) };
                }
            constexpr decltype(auto) operator()(T &t, I const p, seek_origin const o) const {
                return t.seek_write(p, o);
            }

            template<typename T, std::signed_integral I>
                requires requires(T &t, I const p, seek_origin const o) {
                    { seek_write(t, p, o) };
                }
            constexpr decltype(auto) operator()(T &t, I const p, seek_origin const o) const {
                return seek_write(t, p, o);
            }
        };

        void seek_read(auto &, std::signed_integral auto, seek_origin) = delete;

        struct seek_read_fn final {
            template<typename T, std::signed_integral I>
                requires requires(T &t, I const p, seek_origin const o) {
                    { t.seek_read(p, o) };
                }
            constexpr decltype(auto) operator()(T &t, I const p, seek_origin const o) const {
                return t.seek_read(p, o);
            }

            template<typename T, std::signed_integral I>
                requires requires(T &t, I const p, seek_origin const o) {
                    { seek_read(t, p, o) };
                }
            constexpr decltype(auto) operator()(T &t, I const p, seek_origin const o) const {
                return seek_read(t, p, o);
            }
        };

        void flush(auto &) = delete;

        struct flush_fn final {
            template<typename T>
                requires requires(T &t) {
                    { t.flush() };
                }
            constexpr decltype(auto) operator()(T &t) const {
                return t.flush();
            }

            template<typename T>
                requires requires(T &t) {
                    { flush(t) };
                }
            constexpr decltype(auto) operator()(T &t) const {
                return flush(t);
            }
        };

        template<range_of_bytes R>
        void write(auto &, R, std::ranges::range_size_t<R>) = delete;

        void write(auto &, void const *, std::unsigned_integral auto) = delete;

        struct write_fn final {
            template<typename T, std::unsigned_integral I>
                requires requires(T &t, void const *p, I n) {
                    { t.write(p, n) };
                }
            constexpr decltype(auto) operator()(T &t, void const *const p, I const n) const {
                return t.write(p, n);
            }

            template<typename T, std::unsigned_integral I>
                requires requires(T &t, void const *p, I n) {
                    { write(t, p, n) };
                }
            constexpr decltype(auto) operator()(T &t, void const *const p, I const n) const {
                return write(t, p, n);
            }

            template<typename T, range_of_bytes R>
            constexpr decltype(auto) operator()(T &t, R const &r) const {
                return (*this)(t, std::ranges::data(r),
                               sizeof(std::ranges::range_value_t<R>) * std::ranges::size(r));
            }

            template<typename T, range_of_bytes R>
            constexpr decltype(auto) operator()(T &t, R const &r, std::ranges::range_size_t<R> const n) const {
                return (*this)(t, std::ranges::data(r),
                               sizeof(std::ranges::range_value_t<R>) * std::min(n, std::ranges::size(r)));
            }

            template<typename T, typename Ch, std::size_t N>
                requires std::same_as<Ch, char> || std::same_as<Ch, wchar_t> || std::same_as<Ch, char8_t>
                         || std::same_as<Ch, char16_t> || std::same_as<Ch, char32_t> || std::same_as<Ch, std::byte>
                         || std::same_as<Ch, signed char> || std::same_as<Ch, unsigned char>
            constexpr decltype(auto) operator()(T &t, Ch const (&p)[N]) const {
                return (*this)(t, std::ranges::data(p), sizeof(Ch) * (N - 1));
            }
        };

        template<range_of_bytes R>
        std::ranges::range_size_t<R> read(auto &, R &, std::ranges::range_size_t<R>) = delete;

        std::unsigned_integral auto read(auto &, void *, std::unsigned_integral auto) = delete;

        struct read_fn final {
            template<typename T, std::unsigned_integral I>
                requires requires(T &t, void *p, I n) {
                    { t.read(p, n) } -> std::unsigned_integral;
                }
            constexpr std::unsigned_integral auto operator()(T &t, void *p, I n) const {
                return t.read(p, n);
            }

            template<typename T, std::unsigned_integral I>
                requires requires(T &t, void *p, I n) {
                    { read(t, p, n) } -> std::unsigned_integral;
                }
            constexpr std::unsigned_integral auto operator()(T &t, void *p, I const n) const {
                return read(t, p, n);
            }

            template<typename T, range_of_bytes R>
            constexpr std::ranges::range_size_t<R> operator()(T &t, R &r) const {
                return (*this)(t, std::ranges::data(r),
                               sizeof(std::ranges::range_value_t<R>) * std::ranges::size(r));
            }

            template<typename T, range_of_bytes R>
            constexpr std::ranges::range_size_t<R> operator()(T &t, R &r, std::ranges::range_size_t<R> const n) const {
                return (*this)(t, std::ranges::data(r),
                               sizeof(std::ranges::range_value_t<R>) * std::min(n, std::ranges::size(r)));
            }
        };
    }

    inline namespace io_device_cpo {
        // Observations
        constexpr inline io_device_details::is_open_fn is_open{};
        constexpr inline io_device_details::close_fn close{};
        constexpr inline io_device_details::at_eof_fn at_eof{};
        constexpr inline io_device_details::write_position_fn write_position{};
        constexpr inline io_device_details::read_position_fn read_position{};
        constexpr inline io_device_details::read_remaining_fn read_remaining{};
        constexpr inline io_device_details::size_fn size{};
        // Mutations
        constexpr inline io_device_details::truncate_fn truncate{};
        constexpr inline io_device_details::reserve_fn reserve{};
        constexpr inline io_device_details::seek_write_fn seek_write{};
        constexpr inline io_device_details::seek_read_fn seek_read{};
        constexpr inline io_device_details::flush_fn flush{};
        constexpr inline io_device_details::write_fn write{};
        constexpr inline io_device_details::read_fn read{};
    }

    template<typename T>
    concept sink = std::movable<T>
                   && std::is_invocable_v<decltype(write), T &, void const *, std::size_t>
                   && std::is_invocable_v<decltype(write), T &, std::span<std::byte const> >
                   && std::is_invocable_v<decltype(flush), T &>;

    template<typename T>
    concept random_access_sink = sink<T>
                                 && std::is_invocable_v<decltype(write_position), T const &>
                                 && std::is_invocable_v<decltype(size), T const &>
                                 && std::is_invocable_v<decltype(seek_write), T &, std::int64_t, seek_origin>
                                 && std::is_invocable_v<decltype(truncate), T &, std::int64_t>
                                 && std::is_invocable_v<decltype(reserve), T &, std::size_t>;

    template<typename T>
    concept source = std::movable<T>
                     && std::is_invocable_v<decltype(at_eof), T const &>
                     && std::is_invocable_v<decltype(read), T &, void *, std::size_t>
                     && std::is_invocable_v<decltype(read), T &, std::span<std::byte> &>;

    template<typename T>
    concept random_access_source = source<T>
                                   && std::is_invocable_v<decltype(read_position), T const &>
                                   && std::is_invocable_v<decltype(read_remaining), T const &>
                                   && std::is_invocable_v<decltype(size), T const &>
                                   && std::is_invocable_v<decltype(seek_read), T &, std::int64_t, seek_origin>;

    template<typename T>
    concept io_device = std::is_invocable_v<decltype(is_open), T const &>
                        && std::is_invocable_v<decltype(close), T &>
                        && std::is_invocable_v<decltype(size), T const &>;

    template<typename>
    inline constexpr bool has_independent_read_write_pointers_v = false;

    template<random_access_source T>
        requires random_access_sink<T> && requires {
            { T::are_read_write_pointers_independent() } -> boolean;
        }
    inline constexpr bool has_independent_read_write_pointers_v<T> = T::are_read_write_pointers_independent();
} // skizzay
