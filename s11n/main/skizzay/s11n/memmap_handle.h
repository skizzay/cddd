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
    class posix_memmap;

    namespace memmap_details {
        class memmap_handle {
            friend posix_memmap_source;
            friend posix_memmap_sink;
            friend posix_memmap;

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

            [[nodiscard]] std::size_t round_page_up(std::size_t const n) const noexcept {
                return (n + ~page_mask_) & page_mask_;
            }

        private:
            memmap_handle(file_descriptor const &fd, std::size_t const size, auto const permissions,
                          auto const memmap_flags
            ) {
                size_ = round_page_up(size);
                addr_ = mmap(nullptr, size_, permissions, memmap_flags, fd.get(), 0);
                if (MAP_FAILED == addr_) {
                    throw_system_error();
                }
            }

            std::size_t const page_mask_ = ~(static_cast<std::size_t>(sysconf(_SC_PAGESIZE)) - 1);
            void *addr_ = nullptr;
            std::size_t size_ = 0;
        };
    }


    class posix_memmap_source : public source_base<posix_memmap_source> {
        friend source_base;

    public:
        static posix_memmap_source open_shared(std::filesystem::path const &path) {
            return posix_memmap_source{path, MAP_SHARED};
        }

        static posix_memmap_source open_private(std::filesystem::path const &path) {
            return posix_memmap_source{path, MAP_PRIVATE};
        }

        ~posix_memmap_source() {
            close();
        }

        [[nodiscard]] bool is_open() const noexcept {
            return memmap_.is_open();
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
            memmap_ = memmap_details::memmap_handle{
                fd_, static_cast<std::size_t>(initial_size), PROT_READ, memmap_flags
            };
        }

        [[nodiscard]] std::span<std::byte const> read_buffer() const noexcept {
            return {static_cast<std::byte const *>(memmap_.address()) + source_position(),
                    memmap_.size() - source_position()};
        }

        file_descriptor fd_;
        memmap_details::memmap_handle memmap_;
    };


    class posix_memmap_sink : public sink_base<posix_memmap_sink> {
        friend sink_base;

    public:
        static posix_memmap_sink open_shared(std::filesystem::path const &path) {
            return posix_memmap_sink{path, MAP_SHARED};
        }

        static posix_memmap_sink open_private(std::filesystem::path const &path) {
            return posix_memmap_sink{path, MAP_PRIVATE};
        }

        ~posix_memmap_sink() {
            close();
        }

        // ReSharper disable once CppMemberFunctionMayBeConst
        void flush() {
            if (msync(memmap_.address(), sink_position(), MS_SYNC)) {
                throw_system_error();
            }
        }

        [[nodiscard]] bool is_open() const noexcept {
            return memmap_.is_open();
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

    private:
        explicit posix_memmap_sink(std::filesystem::path const &path, auto const memmap_flags)
            : fd_{file_descriptor::open_for_read_write(path)} {
            offset_type const file_size = fd_.fstat().st_size;
            offset_type const initial_size = 0 == file_size ? 1 << 20 : file_size;
            advance(file_size);
            resize_file_by(initial_size);
            memmap_ = memmap_details::memmap_handle{
                fd_, static_cast<std::size_t>(initial_size), PROT_WRITE, memmap_flags
            };
        }

        [[nodiscard]] std::span<std::byte> write_buffer() const noexcept {
            return {static_cast<std::byte *>(memmap_.address()) + sink_position(),
                    memmap_.size() - sink_position()};
        }

        [[nodiscard]] std::size_t write_capacity() const noexcept {
            return memmap_.size();
        }

        // ReSharper disable once CppMemberFunctionMayBeConst
        void resize_file_by(offset_type const n) {
            if (posix_fallocate(fd_.get(), sink_position(), n - sink_position())) {
                throw_system_error();
            }
        }

        void reserve(std::size_t const n) {
            std::size_t const new_size = memmap_.round_page_up(n);
            resize_file_by(static_cast<offset_type>(new_size));
            memmap_.reserve(new_size);
        }

        file_descriptor fd_;
        memmap_details::memmap_handle memmap_;
    };

    class posix_memmap : public sink_base<posix_memmap>,
                         public source_base<posix_memmap> {
        friend sink_base;
        friend source_base;

    public:
        // ReSharper disable once CppRedundantQualifier
        using offset_type = sink_base::offset_type;

        using sink_base::sink_position;
        using source_base::source_position;

        static posix_memmap open_shared(std::filesystem::path const &path) {
            return posix_memmap{path, MAP_SHARED};
        }

        static posix_memmap open_private(std::filesystem::path const &path) {
            return posix_memmap{path, MAP_PRIVATE};
        }

        ~posix_memmap() {
            close();
        }

        [[nodiscard]] bool is_open() const noexcept {
            return memmap_.is_open();
        }

        void close() {
            memmap_.close();
            if (fd_.is_open()) {
                if (ftruncate(fd_.get(), sink_base::sink_position())) {
                    throw_system_error();
                }
                fd_.close();
            }
        }

        // ReSharper disable once CppMemberFunctionMayBeConst
        void flush() {
            if (msync(memmap_.address(), sink_position(), MS_SYNC)) {
                throw_system_error();
            }
        }

    private:
        explicit posix_memmap(std::filesystem::path const &path, auto const memmap_flags)
            : fd_{file_descriptor::open_for_read_write(path)} {
            offset_type const file_size = fd_.fstat().st_size;
            offset_type const initial_size = 0 == file_size ? 1 << 20 : file_size;
            sink_base::advance(file_size);
            resize_file_by(initial_size);
            memmap_ = memmap_details::memmap_handle{
                fd_, static_cast<std::size_t>(initial_size), PROT_WRITE | PROT_READ, memmap_flags
            };
        }

        [[nodiscard]] std::span<std::byte> write_buffer() const noexcept {
            return {static_cast<std::byte *>(memmap_.address()) + sink_position(),
                    (memmap_.size() - sink_position())};
        }

        [[nodiscard]] std::size_t write_capacity() const noexcept {
            return memmap_.size();
        }

        // ReSharper disable once CppMemberFunctionMayBeConst
        void resize_file_by(offset_type const n) {
            if (posix_fallocate(fd_.get(), sink_position(), n - sink_position())) {
                throw_system_error();
            }
        }

        void reserve(std::size_t const n) {
            std::size_t const new_size = memmap_.round_page_up(n);
            resize_file_by(static_cast<offset_type>(new_size));
            memmap_.reserve(new_size);
        }

        [[nodiscard]] std::span<std::byte const> read_buffer() const noexcept {
            return {static_cast<std::byte const *>(memmap_.address()) + source_position(),
                    static_cast<std::size_t>(sink_position() - source_position())};
        }

        file_descriptor fd_;
        memmap_details::memmap_handle memmap_;
    };
} // skizzay

#endif //MEMMAP_HANDLE_H
