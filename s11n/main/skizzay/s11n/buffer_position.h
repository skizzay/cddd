//
// Created by andrew on 2/6/24.
//

#ifndef BUFFER_POSITION_H
#define BUFFER_POSITION_H

#include <cstddef>
#include <stdexcept>
#include <string>

#include "seek_origin.h"

namespace skizzay::s11n {
    enum class position_buffer_type {
        source,
        sink
    };

    template<position_buffer_type T>
    class buffer_position {
    public:
        using offset_type = std::char_traits<char>::off_type;

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
} // skizzay

#endif //BUFFER_POSITION_H
