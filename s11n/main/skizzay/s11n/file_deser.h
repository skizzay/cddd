//
// Created by andrew on 1/25/24.
//

#ifndef FILE_SERIALIZER_H
#define FILE_SERIALIZER_H

#include <system_error>
#include <cstdio>
#include <memory>

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

        constexpr inline auto file_size = [](FILE *f) noexcept -> std::size_t {
            auto const current_position = std::ftell(f);
            std::fseek(f, 0, static_cast<int>(seek_origin::end));
            auto const result = std::ftell(f);
            std::fseek(f, current_position, static_cast<int>(seek_origin::beginning));
            return result;
        };

        template<typename T>
            requires std::is_class_v<T> && std::is_same_v<T, std::remove_cvref_t<T> >
        class file_sink_base {
        public:
            using offset_type = std::char_traits<char>::off_type;

            [[nodiscard]] offset_type sink_position() const noexcept {
                return nullptr == get_file()
                           ? 0
                           : std::ftell(get_file());
            }

            void sink_seek(offset_type const p, seek_origin const origin) {
                if (nullptr != get_file() && std::fseek(get_file(), p, static_cast<int>(origin))) {
                    throw_system_error();
                }
            }

            void set_buffer_size(std::size_t const n) {
                if (nullptr != get_file() && std::setvbuf(get_file(), nullptr, 0 == n ? _IOFBF : _IONBF, n)) {
                    throw_system_error();
                }
            }

            void flush() {
                if (nullptr != get_file() && std::fflush(get_file())) {
                    throw_system_error();
                }
            }

            template<std::ranges::sized_range R>
                requires std::ranges::contiguous_range<R>
            std::size_t write(R const &range) noexcept {
                return nullptr == get_file()
                           ? 0
                           : std::fwrite(std::ranges::data(range), sizeof(std::ranges::range_value_t<R>),
                                         std::ranges::size(range), get_file()) * sizeof(std::ranges::range_value_t<R>);
            }

            template<typename Ch>
                requires std::same_as<Ch, char> || std::same_as<Ch, char8_t>
            std::size_t write(Ch const c) noexcept {
                if (nullptr == get_file()) {
                    return 0;
                }
                if (c != std::fputc(c, get_file())) {
                    throw_system_error();
                }
                return sizeof(Ch);
            }

            std::size_t write(wchar_t const c) noexcept {
                if (nullptr == get_file()) {
                    return 0;
                }
                if (static_cast<std::wint_t>(c) != std::fputwc(c, get_file())) {
                    throw_system_error();
                }
                return sizeof(wchar_t);
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
            template<std::ranges::sized_range R>
                requires std::ranges::contiguous_range<R>
            std::size_t read(R &range, std::size_t const n) noexcept {
                return nullptr == get_file()
                           ? 0
                           : std::fread(
                               std::ranges::data(range),
                               sizeof(std::ranges::range_value_t<R>),
                               std::min(std::ranges::size(std::as_const(range)), n / sizeof(std::ranges::range_value_t<R>)),
                               get_file()) * sizeof(std::ranges::range_value_t<R>);
            }

            [[nodiscard]] offset_type source_position() const noexcept {
                return nullptr == get_file()
                           ? 0
                           : std::ftell(get_file());
            }

            void source_seek(offset_type const p, seek_origin const origin) {
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
        friend class file_sink_base;

    public:
        file_sink() noexcept = default;

        explicit file_sink(char const *filename)
            : f_{std::fopen(filename, "wb+")} {
            sink_seek(0, seek_origin::end);
        }

        static file_sink temporary() {
            file_sink result;
            result.f_.reset(std::tmpfile());
            return result;
        }

        [[nodiscard]] bool is_open() const noexcept {
            return nullptr != get_file_handle();
        }

    private:
        file_handle f_;

        [[nodiscard]] constexpr FILE *get_file_handle() const noexcept {
            return f_.get();
        }
    };

    class file_source : public file_details::file_sink_base<file_source> {
        friend class file_source_base;

    public:
        file_source() noexcept = default;

        explicit file_source(char const *filename)
            : f_{std::fopen(filename, "rb+")} {
        }

        [[nodiscard]] bool is_open() const noexcept {
            return nullptr != get_file_handle();
        }

    private:
        file_handle f_;

        [[nodiscard]] constexpr FILE *get_file_handle() const noexcept {
            return f_.get();
        }
    };

    class file : public file_details::file_sink_base<file>,
                 public file_details::file_source_base<file> {
        friend class file_sink_base;
        friend class file_source_base;

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

        [[nodiscard]] bool is_open() const noexcept {
            return nullptr != get_file_handle();
        }

    private:
        file_handle f_;

        [[nodiscard]] constexpr FILE *get_file_handle() const noexcept {
            return f_.get();
        }
    };
} // skizzay

#endif //FILE_SERIALIZER_H
