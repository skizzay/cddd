//
// Created by andrew on 2/3/24.
//

#ifndef MEMMAP_HANDLE_H
#define MEMMAP_HANDLE_H

#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <cstring>
#include <filesystem>
#include <span>

#include "buffer_position.h"
#include "file_descriptor.h"
#include "seek_origin.h"

// ReSharper disable once CppInconsistentNaming
namespace skizzay::s11n {
    class posix_memmap_source;
    class posix_memmap_sink;

    class memmap_handle {
        friend posix_memmap_source;
        friend posix_memmap_sink;

    public:
        memmap_handle() noexcept = default;

        memmap_handle(memmap_handle const &) = delete;

        memmap_handle(memmap_handle &&other) noexcept
            : page_mask_{other.page_mask_},
              addr_{other.addr_},
              size_{other.size_} {
            other.addr_ = nullptr;
            other.size_ = 0;
        }

        memmap_handle &operator=(memmap_handle const &) = delete;

        memmap_handle &operator=(memmap_handle &&other) noexcept {
            if (this != &other) {
                close();
                addr_ = other.addr_;
                size_ = other.size_;
                other.addr_ = nullptr;
                other.size_ = 0;
            }
            return *this;
        }

        ~memmap_handle() {
            close();
        }

        void reserve(std::size_t const n) {
            if (is_open()) {
                auto const new_size = round_page_up(n);
                void *const new_addr = mremap(addr_, size_, new_size, MREMAP_MAYMOVE);
                if (MAP_FAILED == addr_) {
                    throw_system_error();
                }
                addr_ = new_addr;
                size_ = new_size;
            }
        }

        [[nodiscard]] bool is_open() const noexcept {
            return nullptr != addr_;
        }

        void close() {
            if (is_open()) {
                if (munmap(addr_, size_)) {
                    throw_system_error();
                }
                addr_ = nullptr;
            }
        }

        [[nodiscard]] void *address() const noexcept {
            return addr_;
        }

        [[nodiscard]] std::size_t size() const noexcept {
            return size_;
        }

    private:
        memmap_handle(file_descriptor const &fd, std::size_t const size, auto const permissions, auto const memmap_flags
        ) {
            size_ = round_page_up(size);
            addr_ = mmap(nullptr, size_, permissions, memmap_flags, fd.get(), 0);
            if (MAP_FAILED == addr_) {
                throw_system_error();
            }
        }

        [[nodiscard]] std::size_t round_page_up(std::size_t const n) const noexcept {
            return (n + ~page_mask_) & page_mask_;
        }

        std::size_t const page_mask_ = ~(static_cast<std::size_t>(sysconf(_SC_PAGESIZE)) - 1);
        void *addr_ = nullptr;
        std::size_t size_ = 0;
    };


    class posix_memmap_source : buffer_position<position_buffer_type::source> {
    public:
        using buffer_position::source_position;

        static posix_memmap_source open_shared(std::filesystem::path const &path) {
            return posix_memmap_source{path, MAP_SHARED};
        }

        static posix_memmap_source open_private(std::filesystem::path const &path) {
            return posix_memmap_source{path, MAP_PRIVATE};
        }

        ~posix_memmap_source() {
            close();
        }

        void source_seek(offset_type const p, seek_origin const origin) {
            seek(p, memmap_.size(), origin);
        }

        template<std::ranges::sized_range R>
            requires std::ranges::contiguous_range<R>
        std::size_t read(R &range, std::ranges::range_size_t<R> const n) {
            if (!memmap_.is_open()) {
                return 0;
            }
            std::size_t const num_bytes = std::min(n * sizeof(std::ranges::range_value_t<R>),
                                                   memmap_.size() - source_position());
            std::memcpy(std::ranges::data(range), static_cast<std::byte *>(memmap_.address()) + source_position(),
                        num_bytes);
            advance(num_bytes);
            return num_bytes;
        }

        void close() {
            memmap_.close();
            fd_.close();
        }

    private:
        explicit posix_memmap_source(std::filesystem::path const &path, auto const memmap_flags)
            : fd_{file_descriptor::open_for_read_write(path)} {
            offset_type const file_size = fd_.fstat().st_size;
            offset_type const initial_size = 0 == file_size ? 1 << 20 : file_size;
            advance(file_size);
            memmap_ = memmap_handle{fd_, static_cast<std::size_t>(initial_size), PROT_READ, memmap_flags};
        }

        file_descriptor fd_;
        memmap_handle memmap_;
    };


    class posix_memmap_sink : buffer_position<position_buffer_type::sink> {
    public:
        using buffer_position::sink_position;

        static posix_memmap_sink open_shared(std::filesystem::path const &path) {
            return posix_memmap_sink{path, MAP_SHARED};
        }

        static posix_memmap_sink open_private(std::filesystem::path const &path) {
            return posix_memmap_sink{path, MAP_PRIVATE};
        }

        ~posix_memmap_sink() {
            close();
        }

        template<std::ranges::contiguous_range R>
            requires std::ranges::sized_range<R>
        std::size_t write(R const &range) {
            if (memmap_.is_open()) {
                std::size_t const num_bytes = std::ranges::size(range) * sizeof(std::ranges::range_value_t<R>);
                ensure_size(num_bytes);
                std::memcpy(static_cast<std::byte *>(memmap_.address()) + sink_position(), std::ranges::data(range),
                            num_bytes);
                advance(num_bytes);
                return num_bytes;
            }
            return 0;
        }

        template<typename Ch>
            requires std::same_as<Ch, char> || std::same_as<Ch, char8_t>
        std::size_t write(Ch const c) noexcept {
            if (!memmap_.is_open()) {
                return 0;
            }
            ensure_size(sizeof(Ch));
            static_cast<Ch *>(memmap_.address())[sink_position()] = c;
            advance(sizeof(Ch));
            return sizeof(Ch);
        }

        std::size_t write(wchar_t const c) noexcept {
            if (!memmap_.is_open()) {
                return 0;
            }
            ensure_size(sizeof(wchar_t));
            std::memcpy(static_cast<wchar_t *>(memmap_.address()) + sink_position(), &c, sizeof(wchar_t));
            advance(sizeof(wchar_t));
            return sizeof(wchar_t);
        }

        // ReSharper disable once CppMemberFunctionMayBeConst
        void flush() {
            if (memmap_.is_open() && msync(memmap_.address(), sink_position(), MS_SYNC)) {
                throw_system_error();
            }
        }

        void sink_seek(offset_type const p, seek_origin const origin) {
            seek(p, memmap_.size(), origin);
        }

        void close() {
            memmap_.close();
            if (fd_.is_open()) {
                if (ftruncate(fd_.get(), sink_position())) {
                    throw_system_error();
                }
                fd_.close();
            }
        }

        void ensure_size(std::size_t const n) {
            if (std::size_t const requested_size = sink_position() + n;
                memmap_.is_open() && requested_size > memmap_.size()) {
                resize_file_by(static_cast<offset_type>(n));
                memmap_.reserve(requested_size);
            }
        }

    private:
        explicit posix_memmap_sink(std::filesystem::path const &path, auto const memmap_flags)
            : fd_{file_descriptor::open_for_read_write(path)} {
            offset_type const file_size = fd_.fstat().st_size;
            offset_type const initial_size = 0 == file_size ? 1 << 20 : file_size;
            advance(file_size);
            resize_file_by(initial_size);
            memmap_ = memmap_handle{fd_, static_cast<std::size_t>(initial_size), PROT_WRITE, memmap_flags};
        }

        void resize_file_by(offset_type const n) {
            if (posix_fallocate(fd_.get(), sink_position(), n)) {
                throw_system_error();
            }
        }

        file_descriptor fd_;
        memmap_handle memmap_;
    };
} // skizzay

#endif //MEMMAP_HANDLE_H
