//
// Created by andrew on 2/6/24.
//

#ifndef BUFFER_POSITION_H
#define BUFFER_POSITION_H

#include <cstddef>
#include <stdexcept>

#include <cstring>

#include "seek_origin.h"

namespace skizzay::s11n {
    enum class position_buffer_type {
        source,
        sink
    };

    template<position_buffer_type T>
    class buffer_position {
    public:
        using offset_type = std::make_signed_t<std::size_t>;

        [[nodiscard]] constexpr offset_type sink_position() const noexcept requires(T == position_buffer_type::sink) {
            return position_;
        }

        [[nodiscard]] constexpr offset_type source_position() const noexcept requires(T == position_buffer_type::source) {
            return position_;
        }

    protected:
        constexpr buffer_position() noexcept = default;
        constexpr explicit buffer_position(offset_type const position) noexcept
            : position_{position} {
        }

        constexpr void seek(offset_type const p, std::size_t const size, seek_origin const origin) {
            switch (origin) {
                case seek_origin::beginning:
                    position_ = validate_and_calculate_position(p, 0, size);
                break;

                case seek_origin::current:
                    position_ = validate_and_calculate_position(p, position_, size);
                break;

                case seek_origin::end:
                    position_ = validate_and_calculate_position(p, static_cast<offset_type>(size), size);
                break;

                default:
                    std::unreachable();
            }
        }

        constexpr void advance(offset_type const p) noexcept {
            position_ += p;
        }

    private:
        static constexpr std::size_t validate_and_calculate_position(offset_type const offset,
                                                                     offset_type const position,
                                                                     std::size_t const size
        ) {
            if (offset + position < 0) {
                throw std::out_of_range{"offset + position < 0"};
            }
            if (offset + position > static_cast<offset_type>(size)) {
                throw std::out_of_range{"offset + position > size"};
            }
            return offset + position;
        }

        offset_type position_{};
    };

    template<typename T>
        requires std::is_class_v<T> && std::is_same_v<T, std::remove_cvref_t<T> >
    class source_base : protected buffer_position<position_buffer_type::source> {
    public:
        using buffer_position::source_position;

        void source_seek(offset_type const p, seek_origin const origin) {
            seek(p, read_buffer().size(), origin);
        }

        template<std::ranges::sized_range R>
            requires std::ranges::contiguous_range<R>
        std::size_t read(R &range, std::ranges::range_size_t<R> const n) {
            auto const buffer = read_buffer();
            std::size_t const num_bytes = std::min(n * sizeof(std::ranges::range_value_t<R>),
                                                   buffer.size());
            std::memcpy(std::ranges::data(range), buffer.data(), num_bytes);
            advance(num_bytes);
            return num_bytes;
        }

    private:
        [[nodiscard]] std::span<std::byte const> read_buffer() const noexcept {
            return static_cast<T const *>(this)->read_buffer();
        }
    };

        template<typename T>
            requires std::is_class_v<T> && std::is_same_v<T, std::remove_cvref_t<T> >
        class sink_base : protected buffer_position<position_buffer_type::sink> {
        public:
            using buffer_position::sink_position;

            template<std::ranges::contiguous_range R>
                requires std::ranges::sized_range<R>
            std::size_t write(R const &range) {
                std::size_t const num_bytes = std::ranges::size(range) * sizeof(std::ranges::range_value_t<R>);
                ensure_size(num_bytes);
                std::memcpy(write_buffer().data(), std::ranges::data(range), num_bytes);
                advance(num_bytes);
                return num_bytes;
            }

            template<typename Ch>
                requires (sizeof(std::byte) == sizeof(Ch))
            std::size_t write(Ch const c) noexcept {
                ensure_size(sizeof(Ch));
                *reinterpret_cast<Ch *>(write_buffer().data()) = c;
                advance(sizeof(Ch));
                return sizeof(Ch);
            }

            std::size_t write(wchar_t const c) noexcept {
                ensure_size(sizeof(wchar_t));
                std::memcpy(write_buffer().data(), &c, sizeof(wchar_t));
                advance(sizeof(wchar_t));
                return sizeof(wchar_t);
            }

            void sink_seek(offset_type const p, seek_origin const origin) {
                seek(p, write_capacity(), origin);
            }

            void ensure_size(std::size_t const n) {
                if (std::size_t const requested_size = sink_position() + n;
                    requested_size > write_capacity()) {
                    reserve(requested_size);
                }
            }

        private:
            [[nodiscard]] std::span<std::byte> write_buffer() noexcept {
                return static_cast<T *>(this)->write_buffer();
            }

            [[nodiscard]] std::size_t write_capacity() const noexcept {
                return static_cast<T const *>(this)->write_capacity();
            }

            void reserve(std::size_t const n) {
                static_cast<T *>(this)->reserve(n);
            }
        };
} // skizzay

#endif //BUFFER_POSITION_H
