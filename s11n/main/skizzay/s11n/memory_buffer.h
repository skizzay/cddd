//
// Created by andrew on 1/28/24.
//
#pragma once

#include <cstring>
#include <limits>
#include <vector>

#include "buffer_position.h"
#include "io_device.h"

// ReSharper disable once CppInconsistentNaming
namespace skizzay::s11n {
    template<typename Alloc=std::allocator<std::byte> >
    class memory_buffer final : public buffer_position<position_buffer_type::write>,
                                public buffer_position<position_buffer_type::read> {
    public:
        using buffer_position<position_buffer_type::write>::write_position;
        using buffer_position<position_buffer_type::read>::read_position;

        template<typename... Args>
        explicit memory_buffer(Args &&... args
        ) noexcept(std::is_nothrow_constructible_v<std::vector<std::byte, Alloc>, Args...>)
            : buffer_{std::forward<Args>(args)...} {
        }

        [[nodiscard]] constexpr std::int64_t size() const noexcept {
            return static_cast<std::int64_t>(buffer_.size());
        }

        constexpr void seek_write(std::signed_integral auto const p, seek_origin const origin) {
            this->buffer_position<position_buffer_type::write>::seek(p, size(), origin);
        }

        constexpr std::unsigned_integral auto write(void const * const data, std::unsigned_integral auto const n) {
            auto const position = write_position();
            auto const new_size = static_cast<std::size_t>(position + n);
            if (new_size > buffer_.size()) {
                buffer_.resize(new_size);
            }
            std::memcpy(write_address(), data, n);
            this->buffer_position<position_buffer_type::write>::advance(n);
            return n;
        }

        constexpr void truncate(std::signed_integral auto const n) {
            if (n < 0) {
                throw std::out_of_range{"Cannot truncate to a negative size"};
            }
            if (n < size()) {
                buffer_.resize(static_cast<std::size_t>(n));
            }
        }

        constexpr void reserve(std::unsigned_integral auto const n) {
            buffer_.reserve(n);
        }

        constexpr void flush() noexcept {
            // No-op
        }

        [[nodiscard]] constexpr std::uint64_t read_remaining() const noexcept {
            return static_cast<std::uint64_t>(size() - read_position());
        }

        [[nodiscard]] constexpr bool at_eof() const noexcept {
            return read_position() == write_position();
        }

        constexpr void seek_read(std::signed_integral auto const p, seek_origin const origin) {
            this->buffer_position<position_buffer_type::read>::seek(p, size(), origin);
        }

        constexpr std::unsigned_integral auto read(void * const data, std::unsigned_integral auto const n) noexcept {
            auto const read_bytes = std::min(n, read_remaining());
            std::memcpy(data, read_address(), read_bytes);
            this->buffer_position<position_buffer_type::read>::advance(read_bytes);
            return read_bytes;
        }

        [[nodiscard]] constexpr std::span<std::byte const> read_region(
            std::uint64_t const start = 0,
            std::uint64_t const length = std::numeric_limits<std::uint64_t>::max()
        ) const noexcept {
            auto const new_start = std::min(start, static_cast<std::uint64_t>(write_position()));
            auto const start_address = buffer_.data() + new_start;
            auto const new_length = std::min(length, write_position() - new_start);
            return std::span{start_address, new_length};
        }

    private:
        std::vector<std::byte, Alloc> buffer_;

        [[nodiscard]] void *write_address() noexcept {
            return buffer_.data() + write_position();
        }

        [[nodiscard]] void const *read_address() const noexcept {
            return buffer_.data() + read_position();
        }
    };

    static_assert(random_access_sink<memory_buffer<> >);
    static_assert(random_access_source<memory_buffer<> >);

    class buffer_source final : public buffer_position<position_buffer_type::read> {
    public:
        using buffer_position::read_position;

        explicit buffer_source(std::span<std::byte const> const buffer) noexcept
            : buffer_{buffer} {
        }

        [[nodiscard]] bool at_eof() const noexcept {
            return static_cast<std::size_t>(read_position()) == buffer_.size();
        }

        std::size_t read(void *const data, std::size_t const size) {
            auto const position = read_position();
            auto const n = std::min(buffer_.size() - position, size);
            std::memcpy(data, buffer_.data() + position, n);
            advance(n);
            return n;
        }

        [[nodiscard]] std::uint64_t read_remaining() const noexcept {
            return buffer_.size() - read_position();
        }

        [[nodiscard]] std::int64_t size() const noexcept {
            return static_cast<std::int64_t>(buffer_.size());
        }

        std::int64_t seek_read(std::int64_t const p, seek_origin const origin) noexcept {
            return seek(p, buffer_.size(), origin);
        }

    private:
        std::span<std::byte const> buffer_;
    };
} // skizzay::s11n
