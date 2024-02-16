//
// Created by andrew on 2/3/24.
//

#pragma once

#include <cstring>
#include <sys/mman.h>
#include "buffer_position.h"
#include "file.h"
#include "io_device.h"

// ReSharper disable once CppInconsistentNaming
namespace skizzay::s11n {
    class memory_map : public buffer_position<position_buffer_type::write>,
                       public buffer_position<position_buffer_type::read> {
    public:
        using buffer_position<position_buffer_type::write>::write_position;
        using buffer_position<position_buffer_type::read>::read_position;

        static memory_map open_shared(std::filesystem::path const &path) {
            return memory_map{path, MAP_SHARED};
        }

        static memory_map open_private(std::filesystem::path const &path) {
            return memory_map{path, MAP_PRIVATE};
        }

        memory_map() noexcept = default;

        memory_map(memory_map &&other) noexcept
            : buffer_position<position_buffer_type::write>{other},
              buffer_position<position_buffer_type::read>{other},
              file_{std::move(other.file_)},
              page_mask_{other.page_mask_},
              addr_{other.addr_},
              size_{other.size_} {
            other.addr_ = nullptr;
            other.size_ = 0;
        }

        ~memory_map() noexcept {
            if (is_open()) {
                try {
                    close();
                }
                catch (...) {
                    // Swallow the exception
                }
            }
        }

        // ReSharper disable once CppSpecialFunctionWithoutNoexceptSpecification
        memory_map &operator =(memory_map &&other) {
            // NOLINT(*-noexcept-move-constructor)
            if (this != &other) {
                if (is_open()) {
                    close();
                }
                seek_read(other.read_position(), seek_origin::beginning);
                other.seek_read(0, seek_origin::beginning);
                seek_write(other.write_position(), seek_origin::beginning);
                other.seek_write(0, seek_origin::beginning);
                file_ = std::move(other.file_);
                addr_ = std::exchange(other.addr_, nullptr);
                size_ = std::exchange(other.size_, 0);
            }
            return *this;
        }

        [[nodiscard]] constexpr bool is_open() const noexcept {
            return nullptr != addr_;
        }

        constexpr void close() {
            if (munmap(addr_, size_)) {
                throw_system_error();
            }
            addr_ = nullptr;
            size_ = 0;
            seek_read(0, seek_origin::beginning);
            seek_write(0, seek_origin::beginning);
            file_.close();
        }

        [[nodiscard]] constexpr std::int64_t size() const noexcept {
            return s11n::size(file_);
        }

        constexpr std::int64_t seek_write(std::signed_integral auto const p, seek_origin const origin) {
            return this->buffer_position<position_buffer_type::write>::seek(p, size(), origin);
        }

        constexpr std::unsigned_integral auto write(void const *const data, std::unsigned_integral auto const n) {
            auto const new_position = write_position() + n;
            if (new_position > static_cast<std::size_t>(size())) {
                reserve(new_position);
            }
            std::memcpy(write_address(), data, n);
            this->buffer_position<position_buffer_type::write>::advance(n);
            return n;
        }

        constexpr void truncate(std::signed_integral auto const n) {
            s11n::truncate(file_, n);
        }

        constexpr void reserve(std::unsigned_integral auto const n) {
            if (n > static_cast<std::size_t>(size())) {
                auto const new_size = round_page_up(n);
                file_.reserve(new_size);
                void *const new_addr = mremap(addr_, size_, new_size, MREMAP_MAYMOVE);
                if (MAP_FAILED == addr_) {
                    throw_system_error();
                }
                addr_ = new_addr;
                size_ = new_size;
            }
        }

        // ReSharper disable once CppMemberFunctionMayBeConst
        constexpr void flush() {
            if (msync(addr_, write_position(), MS_SYNC)) {
                throw_system_error();
            }
        }

        [[nodiscard]] constexpr bool at_eof() const noexcept {
            return read_position() == write_position();
        }

        [[nodiscard]] constexpr std::size_t read_remaining() const noexcept {
            return write_position() - read_position();
        }

        constexpr std::int64_t seek_read(std::signed_integral auto const p, seek_origin const origin) {
            return this->buffer_position<position_buffer_type::read>::seek(p, size(), origin);
        }

        constexpr std::unsigned_integral auto read(void *const data, std::unsigned_integral auto const n) {
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
            auto const start_address = static_cast<std::byte const *>(addr_) + new_start;
            auto const new_length = std::min(length, write_position() - new_start);
            return std::span{start_address, new_length};
        }

    private:
        file file_;
        std::size_t const page_mask_ = ~(static_cast<std::size_t>(sysconf(_SC_PAGESIZE)) - 1);
        void *addr_ = nullptr;
        std::size_t size_ = 0;

        explicit memory_map(std::filesystem::path const &path, auto const memmap_flags)
            : file_{file::open(path)} {
            auto const file_size = size();
            auto const initial_size = 0 == file_size ? 1 << 20 : file_size;
            seek_write(0, seek_origin::end);
            size_ = round_page_up(initial_size);
            addr_ = mmap(nullptr, size_, PROT_WRITE | PROT_READ, memmap_flags, file_.native_handle(), 0);
            if (MAP_FAILED == addr_) {
                throw_system_error();
            }
        }

        [[nodiscard]] constexpr std::size_t round_page_up(std::size_t const n) const noexcept {
            return (n + ~page_mask_) & page_mask_;
        }

        [[nodiscard]] void *write_address() const noexcept {
            return static_cast<std::byte *>(addr_) + write_position();
        }

        [[nodiscard]] void const *read_address() const noexcept {
            return static_cast<std::byte const *>(addr_) + read_position();
        }
    };

    static_assert(io_device<memory_map>);
    static_assert(random_access_sink<memory_map>);
    static_assert(random_access_source<memory_map>);
} // skizzay
