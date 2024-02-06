//
// Created by andrew on 1/26/24.
//

#ifndef READ_BUFFER_H
#define READ_BUFFER_H
#include <bit>
#include <concepts>
#include <string>

#include "decode.h"
#include "seek_origin.h"
#include "source.h"

// ReSharper disable once CppInconsistentNaming
namespace skizzay::s11n {
    template<std::endian E, source Source>
    class buffer_reader {
        struct next_result final {
            constexpr explicit next_result(buffer_reader &reader)
                : reader_(reader) {
            }

            template<std::integral I>
            // ReSharper disable once CppNonExplicitConversionOperator
            constexpr operator I() const { // NOLINT(*-explicit-constructor)
                I result{};
                std::array<std::byte, sizeof(I)> bytes{};
                source_read(reader_.source_, bytes);
                decode<E>(bytes, result);
                return result;
            }

        private:
            buffer_reader &reader_;
        };

    public:
        using offset_type = std::char_traits<char>::off_type;

        explicit constexpr buffer_reader(Source source) noexcept(std::is_nothrow_move_constructible_v<Source>)
            : source_{std::move(source)} {
        }

        constexpr buffer_reader &advance(offset_type const n) {
            source_seek(source_, n, seek_origin::current);
            return *this;
        }

        constexpr next_result next() noexcept {
            return next_result{*this};
        }

    private:
        Source source_;
    };
} // skizzay

#endif //READ_BUFFER_H
