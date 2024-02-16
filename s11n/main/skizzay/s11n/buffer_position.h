//
// Created by andrew on 2/6/24.
//

#ifndef BUFFER_POSITION_H
#define BUFFER_POSITION_H

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include "seek_origin.h"
#include "io_device.h"

namespace skizzay::s11n {
    enum class position_buffer_type {
        read,
        write
    };

    template<position_buffer_type T>
    class buffer_position {
    public:
        using offset_type = std::int64_t;

        [[nodiscard]] constexpr offset_type write_position() const noexcept requires(T == position_buffer_type::write) {
            return position_;
        }

        [[nodiscard]] constexpr offset_type read_position() const noexcept requires(T == position_buffer_type::read) {
            return position_;
        }

    protected:
        constexpr buffer_position() noexcept = default;

        constexpr offset_type seek(offset_type const p, std::size_t const size, seek_origin const origin) {
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

            return position_;
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

    template<random_access_source T>
        requires random_access_sink<T> && (!requires {
            { T::are_read_write_pointers_independent() } -> boolean;
        })
    inline constexpr bool has_independent_read_write_pointers_v<T> =
            std::derived_from<T, buffer_position<position_buffer_type::read> >
            && std::derived_from<T, buffer_position<position_buffer_type::write> >;
} // skizzay

#endif //BUFFER_POSITION_H
