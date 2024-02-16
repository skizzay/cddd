//
// Created by andrew on 1/25/24.
//

#pragma once

#include <filesystem>
#include <sys/stat.h>

#include "throw_system_error.h"
#if __has_include(<fcntl.h>)
#include <fcntl.h>
#include <unistd.h>
constexpr inline bool has_posix_fallocate_ = _POSIX_C_SOURCE >= 200112L;
#else
constexpr inline bool has_posix_fallocate_ = false;
#endif

#include "io_device.h"
#include "seek_origin.h"

// ReSharper disable once CppInconsistentNaming
namespace skizzay::s11n {
    class file {
    public:
        file() noexcept = default;
        file(file const &) = delete;
        file(file &&other) noexcept
            : fd_{std::exchange(other.fd_, -1)} {
        }

        ~file() noexcept {
            if (is_open()) {
                try {
                    close();
                }
                catch (...) {
                    // Swallow the exception
                }
            }
        }

        file & operator = (file const &) = delete;
        // ReSharper disable once CppSpecialFunctionWithoutNoexceptSpecification
        file & operator = (file &&other) { // NOLINT(*-noexcept-move-constructor)
            if (this != &other) {
                if (is_open()) {
                    close();
                }
                std::swap(this->fd_, other.fd_);
            }
            return *this;
        }

        [[nodiscard]] static file open(std::filesystem::path const &filename) {
            return open(filename.c_str());
        }

        [[nodiscard]] static file open(char const *filename) {
            return file{::open(filename, O_RDWR | O_CREAT, permissions)};
        }

        [[nodiscard]] static file temporary() {
            return file{
                ::open(std::filesystem::temp_directory_path().c_str(), O_TMPFILE | O_RDWR, permissions)
            };
        }

        [[nodiscard]] bool is_open() const noexcept {
            return 0 <= fd_;
        }

        void close() {
            if (-1 == ::close(fd_)) {
                throw_system_error();
            }
            fd_ = -1;
        }

        [[nodiscard]] constexpr static bool are_read_write_pointers_independent() noexcept {
            return false;
        }

        [[nodiscard]] std::int64_t write_position() const noexcept {
            return position();
        }

        void seek_write(std::signed_integral auto const p, seek_origin const origin) {
            seek(p, origin);
        }

        [[nodiscard]] auto size() const {
            struct ::stat st{};
            if (::fstat(fd_, &st)) {
                throw_system_error();
            }
            return st.st_size;
        }

        std::unsigned_integral auto write(void const *const data, std::unsigned_integral auto const n) {
            std::signed_integral auto const result = ::write(fd_, data, n);
            if (result < 0) {
                throw_system_error();
            }
            return static_cast<std::make_unsigned_t<decltype(result)>>(result);
        }

        void truncate(std::signed_integral auto const n) {
            if (::ftruncate(fd_, n)) {
                throw_system_error();
            }
        }

        void reserve(std::unsigned_integral auto const n) {
            if (auto const result = ::posix_fallocate(fd_, 0, n); result) {
                throw_system_error(result);
            }
        }

        // ReSharper disable once CppMemberFunctionMayBeConst
        void flush() { // NOLINT(*-make-member-function-const)
            if (::fsync(fd_)) {
                throw_system_error();
            }
        }

        [[nodiscard]] std::int64_t read_position() const noexcept {
            return position();
        }

        [[nodiscard]] bool at_eof() const {
            return read_position() == size();
        }

        void seek_read(std::signed_integral auto const p, seek_origin const origin) {
            seek(p, origin);
        }

        std::unsigned_integral auto read(void * const data, std::unsigned_integral auto const n) {
            std::signed_integral auto const result = ::read(fd_, data, n);
            if (result < 0) {
                throw_system_error();
            }
            return static_cast<std::make_unsigned_t<decltype(result)>>(result);
        }

        [[nodiscard]] int native_handle() const noexcept {
            return fd_;
        }

    private:
        static constexpr auto permissions = S_IRUSR | S_IWUSR;

        explicit file(int const fd) noexcept
            : fd_{fd} {
            // ReSharper disable once CppDFAConstantConditions
            if (0 > fd_) {
                // ReSharper disable once CppDFAUnreachableCode
                throw_system_error();
            }
        }

        [[nodiscard]] std::int64_t position() const noexcept {
            return ::lseek(fd_, 0, SEEK_CUR);
        }

        void seek(std::signed_integral auto const p, seek_origin const origin) {
            if (int const whence = [](seek_origin const o) noexcept {
                switch (o) {
                    case seek_origin::beginning:
                        return SEEK_SET;
                    case seek_origin::current:
                        return SEEK_CUR;
                    case seek_origin::end:
                        return SEEK_END;
                    default:
                        std::unreachable();
                }
            }(origin); ::lseek(fd_, p, whence) < 0) {
                throw_system_error();
            }
        }

        int fd_{-1};
    };

    static_assert(io_device<file>);
    static_assert(random_access_sink<file>);
    static_assert(random_access_source<file>);
} // skizzay
