//
// Created by andrew on 2/12/24.
//

#pragma once

#include <algorithm>
#include <bit>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <span>
#include "range_of_bytes.h"
#include "io_device.h"

namespace skizzay::s11n {
    template<source Source, std::endian E = std::endian::native>
    class binary_reader {
    public:
        explicit binary_reader(Source &source) noexcept
            : source_{source} {
        }

        auto position() const noexcept
            requires random_access_source<Source> {
            return read_position(source_);
        }

        std::int64_t skip(std::int64_t const p)
            requires random_access_source<Source> {
            auto const current = position();
            seek_read(source_, p, seek_origin::current);
            return position() - current;
        }

        friend binary_reader &operator>>(binary_reader &reader, range_of_bytes auto &bytes)
            requires (sizeof(std::ranges::range_value_t<decltype(bytes)>) == 1) || (E == std::endian::native) {
            s11n::read(reader.source_, bytes);
            return reader;
        }

        friend binary_reader &operator>>(binary_reader &reader, std::integral auto &i) {
            auto *const data = reinterpret_cast<std::byte *>(&i);
            s11n::read(reader.source_, data, sizeof(i));
            if constexpr (E != std::endian::native) {
                i = std::byteswap(i);
            }
            return reader;
        }

        template<typename I>
        requires std::is_enum_v<I>
        friend binary_reader &operator>>(binary_reader &reader, I &i) {
            std::underlying_type_t<I> value;
            reader >> value;
            i = static_cast<I>(value);
            return reader;
        }

        friend binary_reader &operator>>(binary_reader &reader, bool &b) {
            std::uint8_t i;
            reader >> i;
            b = i != 0;
            return reader;
        }

        friend binary_reader &operator>>(binary_reader &reader, std::floating_point auto &f) {
            auto *const data = reinterpret_cast<std::byte *>(&f);
            s11n::read(reader.source_, data, sizeof(f));
            if constexpr (E != std::endian::native) {
                std::ranges::reverse(std::span{data, sizeof(f)});
            }
            return reader;
        }

        template<typename Rep, typename Period>
        friend binary_reader &operator>>(binary_reader &reader, std::chrono::duration<Rep, Period> &d) {
            Rep count;
            reader >> count;
            d = std::chrono::duration<Rep, Period>{count};
            return reader;
        }

        template<typename Clock, typename Duration>
        friend binary_reader &operator>>(binary_reader &reader, std::chrono::time_point<Clock, Duration> &tp) {
            Duration d;
            reader >> d;
            tp = std::chrono::time_point<Clock, Duration>{d};
            return reader;
        }

    private:
        Source &source_;
    };

    template<source S>
    using big_endian_binary_reader = binary_reader<S, std::endian::big>;
    template<source S>
    using little_endian_binary_reader = binary_reader<S, std::endian::little>;
    template<source S>
    using native_endian_binary_reader = binary_reader<S, std::endian::native>;
} // skizzay
