//
// Created by andrew on 1/25/24.
//

#ifndef FILE_SERIALIZER_H
#define FILE_SERIALIZER_H

#include <system_error>
#include <cstdio>
#include <memory>
#include <span>

#include "seek_origin.h"

// ReSharper disable once CppInconsistentNaming
namespace skizzay::s11n { namespace file_details {
        constexpr inline auto throw_system_error = [][[noreturn]] {
            throw std::system_error{errno, std::system_category()};
        };

        constexpr inline auto close_file = [](FILE *f) {
            if (nullptr != f && std::fclose(f)) {
                throw_system_error();
            }
        };

        template<typename T>
            requires std::is_class_v<T> && std::is_same_v<T, std::remove_cvref_t<T> >
        class file_sink_base {
        public:
            using offset_type = std::char_traits<char>::off_type;

            [[nodiscard]] offset_type write_position() const noexcept {
                return nullptr == get_file()
                           ? 0
                           : std::ftell(get_file());
            }

            void seek_write(offset_type const p, seek_origin const origin) {
                if (nullptr != get_file() && std::fseek(get_file(), p, static_cast<int>(origin))) {
                    throw_system_error();
                }
            }

            void set_buffer_size(std::size_t const n) noexcept {
                if (nullptr != get_file() && std::setvbuf(get_file(), nullptr, 0 == n ? _IOFBF : _IONBF, n)) {
                    throw_system_error();
                }
            }

            void flush() noexcept {
                if (nullptr != get_file() && std::fflush(get_file())) {
                    throw_system_error();
                }
            }

            template<std::size_t N>
            std::size_t write(std::span<std::byte const, N> bytes) {
                return nullptr == get_file()
                           ? 0
                           : std::fwrite(bytes.data(), sizeof(std::byte), bytes.size(), get_file());
            }

            template<typename Ch=char, typename Tr=std::char_traits<Ch>>
            std::size_t write(std::basic_string_view<Ch, Tr> const str) {
                return nullptr == get_file()
                           ? 0
                           : std::fwrite(str.data(), sizeof(Ch), str.size(), get_file());
            }

        protected:
            using file_handle = std::unique_ptr<FILE, decltype(close_file)>;

        private:
            [[nodiscard]] constexpr FILE *get_file() const noexcept {
                return static_cast<T const *>(this)->get_file_handle();
            }
        };

        template<typename T>
        class file_source_base {
        public:
            using offset_type = std::char_traits<char>::off_type;

            std::size_t read(std::span<std::byte> &bytes) {
                return nullptr == get_file()
                           ? 0
                           : std::fread(bytes.data(), sizeof(std::byte), bytes.size(), get_file());
            }

            [[nodiscard]] offset_type read_position() const noexcept {
                return nullptr == get_file()
                           ? 0
                           : std::ftell(get_file());
            }

            void seek_read(offset_type const p, seek_origin const origin) {
                if (nullptr != get_file() && std::fseek(get_file(), p, static_cast<int>(origin))) {
                    throw_system_error();
                }
            }

        private:
            [[nodiscard]] constexpr FILE *get_file() const noexcept {
                return static_cast<T const *>(this)->get_file_handle();
            }
        };
    }

    class file_sink : public file_details::file_sink_base<file_sink> {
    public:
        file_sink() noexcept = default;

        explicit file_sink(char const *filename)
            : f_{std::fopen(filename, "wb+")} {
        }

        static file_sink temporary() {
            file_sink result;
            result.f_.reset(std::tmpfile());
            return result;
        }

    private:
        file_handle f_;

        [[nodiscard]] constexpr FILE *get_file_handle() const noexcept {
            return f_.get();
        }
    };

    class file_source : public file_details::file_sink_base<file_source> {
    public:
        file_source() noexcept = default;

        explicit file_source(char const *filename)
            : f_{std::fopen(filename, "rb+")} {
        }

    private:
        file_handle f_;

        [[nodiscard]] constexpr FILE *get_file_handle() const noexcept {
            return f_.get();
        }
    };

    class file : public file_details::file_sink_base<file>,
                 public file_details::file_source_base<file> {
    public:
        file() noexcept = default;

        explicit file(char const *filename)
            : f_{std::fopen(filename, "rwb+")} {
        }

        static file temporary() {
            file result;
            result.f_.reset(std::tmpfile());
            return result;
        }

    private:
        file_handle f_;

        [[nodiscard]] constexpr FILE *get_file_handle() const noexcept {
            return f_.get();
        }
    };
} // skizzay

#endif //FILE_SERIALIZER_H
