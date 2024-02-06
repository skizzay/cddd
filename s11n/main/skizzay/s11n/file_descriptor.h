//
// Created by andrew on 2/5/24.
//

#ifndef FILE_DESCRIPTOR_H
#define FILE_DESCRIPTOR_H

#include <cstring>
#include <filesystem>
#include "throw_system_error.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

namespace skizzay::s11n {
    class file_descriptor final {
    public:
        static file_descriptor open_for_write(std::filesystem::path const &path) {
            return file_descriptor{::open(path.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)};
        }

        static file_descriptor open_for_read(std::filesystem::path const &path) {
            return file_descriptor{::open(path.c_str(), O_RDONLY, S_IRUSR | S_IWUSR)};
        }

        static file_descriptor open_for_read_write(std::filesystem::path const &path) {
            return file_descriptor{::open(path.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)};
        }

        file_descriptor() noexcept = default;

        file_descriptor(file_descriptor const &) = delete;

        file_descriptor(file_descriptor &&other) noexcept
            : fd_{other.fd_} {
            other.fd_ = -1;
        }

        file_descriptor &operator=(file_descriptor const &) = delete;

        // ReSharper disable once CppSpecialFunctionWithoutNoexceptSpecification
        file_descriptor &operator=(file_descriptor &&other) { // NOLINT(*-noexcept-move-constructor)
            if (this != &other) {
                close();
                fd_ = other.fd_;
                other.fd_ = -1;
            }
            return *this;
        }

        ~file_descriptor() noexcept(false) {
            close();
        }

        [[nodiscard]] bool is_open() const noexcept {
            return -1 != fd_;
        }

        [[nodiscard]] int get() const noexcept {
            return fd_;
        }

        void close() {
            if (is_open()) {
                if (::close(fd_)) {
                    throw_system_error();
                }
                fd_ = -1;
            }
        }

        [[nodiscard]] struct stat fstat() const {
            struct stat st{};
            if (!is_open()) {
                std::memset(&st, 0, sizeof(st));
                return st;
            }
            if (::fstat(fd_, &st)) {
                throw_system_error();
            }
            return st;
        }

    private:
        explicit file_descriptor(int const fd)
            : fd_{fd} {
            if (!is_open()) {
                throw_system_error();
            }
        }

        int fd_ = -1;
    };
} // skizzay

#endif //FILE_DESCRIPTOR_H
