//
// Created by andrew on 1/28/24.
//

#ifndef MEMORY_BUFFER_H
#define MEMORY_BUFFER_H
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include "buffer_position.h"


// ReSharper disable once CppInconsistentNaming
namespace skizzay::s11n {
    template<typename Alloc=std::allocator<std::byte> >
    class memory_buffer final : public sink_base<memory_buffer<Alloc> >,
                                public source_base<memory_buffer<Alloc> > {
        friend sink_base<memory_buffer>;
        friend source_base<memory_buffer>;

    public:
        using sink_base<memory_buffer>::sink_position;
        using source_base<memory_buffer>::source_position;

        template<typename... Args>
        explicit memory_buffer(Args &&... args
        ) noexcept(std::is_nothrow_constructible_v<std::vector<std::byte, Alloc>, Args...>)
            : buffer_{std::forward<Args>(args)...} {
        }

        [[nodiscard]] std::size_t size() const noexcept {
            return buffer_.size();
        }

        // ReSharper disable once CppMemberFunctionMayBeStatic
        void flush() noexcept {
            // No-op
        }

    private:
        [[nodiscard]] std::span<std::byte> write_buffer() noexcept {
            return {buffer_.data() + this->sink_position(), buffer_.size() - this->sink_position()};
        }

        [[nodiscard]] std::size_t write_capacity() const noexcept {
            return buffer_.size();
        }

        void reserve(std::size_t const n) {
            // Allocate at least n bytes, letting std::vector choose the growth strategy
            // Then resize to n to capture the new capacity
            buffer_.reserve(n);
            buffer_.resize(n);
        }

        [[nodiscard]] std::span<std::byte const> read_buffer() const noexcept {
            return {buffer_.data() + this->source_position(), buffer_.size() - this->source_position()};
        }

        std::vector<std::byte, Alloc> buffer_;
    };
} // skizzay

#endif //MEMORY_BUFFER_H
