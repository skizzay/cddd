//
// Created by andrew on 2/9/24.
//

#pragma once

#include <bit>
#include <chrono>

#include "skizzay/s11n/io_device.h"
#include "skizzay/s11n/encode.h"
#include "skizzay/s11n/range_of_bytes.h"

namespace skizzay::s11n {
    template<sink Sink, std::endian E = std::endian::native>
    class binary_writer {
    public:
        explicit binary_writer(Sink &sink) noexcept
            : sink_{sink} {
        }

        ~binary_writer() noexcept {
            try {
                s11n::flush(sink_);
            }
            catch (...) {
                // suppress exceptions
            }
        }

        std::uint64_t write(void const *const data, std::uint64_t const size) {
            return s11n::write(sink_, data, size);
        }

        binary_writer &seek_end()
            requires random_access_sink<Sink> {
            s11n::seek_write(sink_, 0, seek_origin::end);
            return *this;
        }

        friend binary_writer &operator<<(binary_writer &writer, range_of_bytes auto const bytes)
            requires (sizeof(std::ranges::range_value_t<decltype(bytes)>) == 1) || (E == std::endian::native) {
            s11n::write(writer.sink_, bytes);
            return writer;
        }

        friend binary_writer &operator<<(binary_writer &writer, std::integral auto i) {
            if constexpr (E != std::endian::native) {
                i = std::byteswap(i);
            }
            s11n::write(writer.sink_, &i, sizeof(i));
            return writer;
        }

        friend binary_writer &operator<<(binary_writer &writer, std::floating_point auto const f) {
            if constexpr (E == std::endian::native) {
                s11n::write(writer.sink_, &f, sizeof(f));
            }
            else {
                auto encoded = std::bit_cast<std::array<std::byte, sizeof(f)> >(f);
                std::ranges::reverse(encoded);
                s11n::write(writer.sink_, encoded);
            }
            return writer;
        }

        template<typename Enum>
            requires std::is_enum_v<Enum>
        friend binary_writer &operator<<(binary_writer &writer, Enum const e) {
            return writer << std::to_underlying(e);
        }

        friend binary_writer &operator<<(binary_writer &writer, bool const b) {
            return writer << static_cast<std::uint8_t>(b);
        }

        template<typename Ch, typename Tr>
        friend binary_writer &operator<<(binary_writer &writer, std::basic_string_view<Ch, Tr> const s) {
            s11n::write(writer.sink_, s.data(), s.size());
            return writer;
        }

        template<typename Rep, typename Period>
        friend binary_writer &operator<<(binary_writer &writer, std::chrono::duration<Rep, Period> const &d) {
            return writer << d.count();
        }

        template<typename Clock, typename Duration>
        friend binary_writer &operator<<(binary_writer &writer, std::chrono::time_point<Clock, Duration> const &tp) {
            return writer << tp.time_since_epoch();
        }

    private:
        Sink &sink_;
    };

    template<sink S>
    using big_endian_binary_writer = binary_writer<S, std::endian::big>;
    template<sink S>
    using little_endian_binary_writer = binary_writer<S, std::endian::little>;
    template<sink S>
    using native_endian_binary_writer = binary_writer<S, std::endian::native>;
} // skizzay
