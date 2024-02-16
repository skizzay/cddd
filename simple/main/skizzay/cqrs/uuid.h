//
// Created by andrew on 1/18/24.
//

#ifndef UUID_H
#define UUID_H

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <functional>
#include <random>
#include <span>

namespace skizzay::simple::cqrs {
    class uuid {
        friend class std::hash<uuid>;

    public:
        enum class variant : std::uint8_t {
            rfc4122 = 0,
            reserved = 1,
            microsoft = 2,
            future = 3,
            invalid = 4
        };

        constexpr uuid() noexcept = default;

        constexpr uuid(uuid const &) noexcept = default;

        constexpr uuid &operator=(uuid const &) noexcept = default;

        constexpr uuid(uuid &&) noexcept = default;

        constexpr uuid &operator=(uuid &&) noexcept = default;

        constexpr ~uuid() noexcept = default;

        constexpr bool operator==(uuid const &rhs) const noexcept {
            return bytes_ == rhs.bytes_;
        }

        constexpr bool operator!=(uuid const &rhs) const noexcept {
            return !(*this == rhs);
        }

        constexpr auto operator<=>(uuid const &rhs) const noexcept = default;

        [[nodiscard]] constexpr std::uint32_t time_low() const noexcept {
            return static_cast<std::uint32_t>(bytes_[0]) << 24 |
                   static_cast<std::uint32_t>(bytes_[1]) << 16 |
                   static_cast<std::uint32_t>(bytes_[2]) << 8 |
                   static_cast<std::uint32_t>(bytes_[3]);
        }

        [[nodiscard]] constexpr std::uint16_t time_mid() const noexcept {
            return static_cast<std::uint16_t>(bytes_[4]) << 8 |
                   static_cast<std::uint16_t>(bytes_[5]);
        }

        [[nodiscard]] constexpr std::uint16_t time_hi_and_version() const noexcept {
            return static_cast<std::uint16_t>(bytes_[6]) << 8 |
                   static_cast<std::uint16_t>(bytes_[7]);
        }

        [[nodiscard]] constexpr std::uint8_t clock_seq_hi_and_reserved() const noexcept {
            return static_cast<std::uint8_t>(bytes_[8]);
        }

        [[nodiscard]] constexpr std::uint8_t clock_seq_low() const noexcept {
            return static_cast<std::uint8_t>(bytes_[9]);
        }

        [[nodiscard]] constexpr auto node() const noexcept {
            return std::span{std::ranges::data(bytes_) + 10, 6};
        }

        [[nodiscard]] constexpr std::uint8_t version() const noexcept {
            return (time_hi_and_version() & 0xf000) >> 12;
        }

        [[nodiscard]] constexpr variant variant() const noexcept {
            switch (static_cast<int>(bytes_[8] >> 4)) {
                case 0x00: [[fallthrough]];
                case 0x01: [[fallthrough]];
                case 0x02: [[fallthrough]];
                case 0x03: [[fallthrough]];
                case 0x04: [[fallthrough]];
                case 0x05: [[fallthrough]];
                case 0x06: [[fallthrough]];
                case 0x07:
                    return variant::reserved;

                case 0x08: [[fallthrough]];
                case 0x09: [[fallthrough]];
                case 0x0a: [[fallthrough]];
                case 0x0b:
                    return variant::rfc4122;

                case 0x0c: [[fallthrough]];
                case 0x0d:
                    return variant::microsoft;

                case 0x0e:
                    return variant::future;

                default:
                    return variant::invalid;
            }
        }

        [[nodiscard]] constexpr bool is_nil() const noexcept {
            return *this == nil();
        }

        [[nodiscard]] constexpr bool is_valid() const noexcept {
            return variant() != variant::invalid;
        }

        [[nodiscard]] constexpr std::string to_string() const {
            constexpr auto hex = std::array<std::string_view, 256>{
                "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0a", "0b", "0c", "0d", "0e", "0f",
                "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1a", "1b", "1c", "1d", "1e", "1f",
                "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2a", "2b", "2c", "2d", "2e", "2f",
                "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3a", "3b", "3c", "3d", "3e", "3f",
                "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4a", "4b", "4c", "4d", "4e", "4f",
                "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5a", "5b", "5c", "5d", "5e", "5f",
                "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6a", "6b", "6c", "6d", "6e", "6f",
                "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7a", "7b", "7c", "7d", "7e", "7f",
                "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8a", "8b", "8c", "8d", "8e", "8f",
                "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9a", "9b", "9c", "9d", "9e", "9f",
                "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "aa", "ab", "ac", "ad", "ae", "af",
                "b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf",
                "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "ca", "cb", "cc", "cd", "ce", "cf",
                "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "da", "db", "dc", "dd", "de", "df",
                "e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7", "e8", "e9", "ea", "eb", "ec", "ed", "ee", "ef",
                "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "fa", "fb", "fc", "fd", "fe", "ff"
            };
            std::string result{};
            result.reserve(36);
            result += hex[static_cast<std::uint8_t>(bytes_[0])];
            result += hex[static_cast<std::uint8_t>(bytes_[1])];
            result += hex[static_cast<std::uint8_t>(bytes_[2])];
            result += hex[static_cast<std::uint8_t>(bytes_[3])];
            result += '-';
            result += hex[static_cast<std::uint8_t>(bytes_[4])];
            result += hex[static_cast<std::uint8_t>(bytes_[5])];
            result += '-';
            result += hex[static_cast<std::uint8_t>(bytes_[6])];
            result += hex[static_cast<std::uint8_t>(bytes_[7])];
            result += '-';
            result += hex[static_cast<std::uint8_t>(bytes_[8])];
            result += hex[static_cast<std::uint8_t>(bytes_[9])];
            result += '-';
            result += hex[static_cast<std::uint8_t>(bytes_[10])];
            result += hex[static_cast<std::uint8_t>(bytes_[11])];
            result += hex[static_cast<std::uint8_t>(bytes_[12])];
            result += hex[static_cast<std::uint8_t>(bytes_[13])];
            result += hex[static_cast<std::uint8_t>(bytes_[14])];
            result += hex[static_cast<std::uint8_t>(bytes_[15])];
            return result;
        }

        [[nodiscard]] constexpr std::wstring to_wstring() const {
            constexpr auto hex = std::array<std::wstring_view, 256>{
                L"00", L"01", L"02", L"03", L"04", L"05", L"06", L"07", L"08", L"09", L"0a", L"0b", L"0c", L"0d", L"0e", L"0f",
                L"10", L"11", L"12", L"13", L"14", L"15", L"16", L"17", L"18", L"19", L"1a", L"1b", L"1c", L"1d", L"1e", L"1f",
                L"20", L"21", L"22", L"23", L"24", L"25", L"26", L"27", L"28", L"29", L"2a", L"2b", L"2c", L"2d", L"2e", L"2f",
                L"30", L"31", L"32", L"33", L"34", L"35", L"36", L"37", L"38", L"39", L"3a", L"3b", L"3c", L"3d", L"3e", L"3f",
                L"40", L"41", L"42", L"43", L"44", L"45", L"46", L"47", L"48", L"49", L"4a", L"4b", L"4c", L"4d", L"4e", L"4f",
                L"50", L"51", L"52", L"53", L"54", L"55", L"56", L"57", L"58", L"59", L"5a", L"5b", L"5c", L"5d", L"5e", L"5f",
                L"60", L"61", L"62", L"63", L"64", L"65", L"66", L"67", L"68", L"69", L"6a", L"6b", L"6c", L"6d", L"6e", L"6f",
                L"70", L"71", L"72", L"73", L"74", L"75", L"76", L"77", L"78", L"79", L"7a", L"7b", L"7c", L"7d", L"7e", L"7f",
                L"80", L"81", L"82", L"83", L"84", L"85", L"86", L"87", L"88", L"89", L"8a", L"8b", L"8c", L"8d", L"8e", L"8f",
                L"90", L"91", L"92", L"93", L"94", L"95", L"96", L"97", L"98", L"99", L"9a", L"9b", L"9c", L"9d", L"9e", L"9f",
                L"a0", L"a1", L"a2", L"a3", L"a4", L"a5", L"a6", L"a7", L"a8", L"a9", L"aa", L"ab", L"ac", L"ad", L"ae", L"af",
                L"b0", L"b1", L"b2", L"b3", L"b4", L"b5", L"b6", L"b7", L"b8", L"b9", L"ba", L"bb", L"bc", L"bd", L"be", L"bf",
                L"c0", L"c1", L"c2", L"c3", L"c4", L"c5", L"c6", L"c7", L"c8", L"c9", L"ca", L"cb", L"cc", L"cd", L"ce", L"cf",
                L"d0", L"d1", L"d2", L"d3", L"d4", L"d5", L"d6", L"d7", L"d8", L"d9", L"da", L"db", L"dc", L"dd", L"de", L"df",
                L"e0", L"e1", L"e2", L"e3", L"e4", L"e5", L"e6", L"e7", L"e8", L"e9", L"ea", L"eb", L"ec", L"ed", L"ee", L"ef",
                L"f0", L"f1", L"f2", L"f3", L"f4", L"f5", L"f6", L"f7", L"f8", L"f9", L"fa", L"fb", L"fc", L"fd", L"fe", L"ff"
            };
            std::wstring result{};
            result.reserve(36);
            result += hex[static_cast<std::uint8_t>(bytes_[0])];
            result += hex[static_cast<std::uint8_t>(bytes_[1])];
            result += hex[static_cast<std::uint8_t>(bytes_[2])];
            result += hex[static_cast<std::uint8_t>(bytes_[3])];
            result += L'-';
            result += hex[static_cast<std::uint8_t>(bytes_[4])];
            result += hex[static_cast<std::uint8_t>(bytes_[5])];
            result += L'-';
            result += hex[static_cast<std::uint8_t>(bytes_[6])];
            result += hex[static_cast<std::uint8_t>(bytes_[7])];
            result += L'-';
            result += hex[static_cast<std::uint8_t>(bytes_[8])];
            result += hex[static_cast<std::uint8_t>(bytes_[9])];
            result += L'-';
            result += hex[static_cast<std::uint8_t>(bytes_[10])];
            result += hex[static_cast<std::uint8_t>(bytes_[11])];
            result += hex[static_cast<std::uint8_t>(bytes_[12])];
            result += hex[static_cast<std::uint8_t>(bytes_[13])];
            result += hex[static_cast<std::uint8_t>(bytes_[14])];
            result += hex[static_cast<std::uint8_t>(bytes_[15])];
            return result;
        }

        constexpr std::span<std::byte const, 16> bytes() const noexcept {
            return bytes_;
        }

        constexpr static uuid nil() noexcept {
            return uuid{};
        }

        constexpr static uuid max() noexcept {
            uuid result{};
            std::ranges::fill(result.bytes_, std::byte{255});
            return result;
        }

        constexpr static uuid v4(std::uniform_random_bit_generator auto &generator) noexcept {
            std::uniform_int_distribution<std::uint8_t> dist;
            uuid result{};
            std::ranges::generate(result.bytes_, [&]() {
                return std::byte{dist(generator)};
            });
            result.set_version(4);
            // TODO: This is not correct. We need to fix the variant.
            result.bytes_[8] = (result.bytes_[8] & std::byte{0x0f}) | std::byte{0x10};
            return result;
        }

        constexpr static uuid v4() noexcept {
            return v4(random_generator());
        }

        constexpr static uuid v7(std::chrono::milliseconds const time,
                                 std::uniform_random_bit_generator auto &generator
        ) noexcept {
            std::uniform_int_distribution<std::uint8_t> dist;
            uuid result{};
            auto const t = static_cast<std::uint64_t>(time.count()) & 0x0000FFFFFFFFFFFF;
            if constexpr (std::endian::native == std::endian::little) {
                std::ranges::copy_n(reinterpret_cast<std::byte const *>(&t), 6, std::begin(result.bytes_));
            }
            else {
                std::ranges::copy_n(reinterpret_cast<std::byte const *>(&t) + 4, 2, std::begin(result.bytes_));
                std::ranges::copy_n(reinterpret_cast<std::byte const *>(&t), 4, std::begin(result.bytes_) + 4);
            }
            std::ranges::generate_n(std::begin(result.bytes_) + 6, 10, [&]() {
                return std::byte{dist(generator)};
            });
            result.set_version(7);
            result.bytes_[8] = (result.bytes_[8] & std::byte{0x3f}) | std::byte{0x80};
            return result;
        }

        static uuid v7(std::chrono::milliseconds const time) noexcept {
            return v7(time, random_generator());
        }

        static uuid v7() noexcept {
            return v7(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()));
        }

        constexpr static uuid from_bytes(std::span<std::byte const, 16> const bytes) noexcept {
            uuid result{};
            std::ranges::copy(bytes, std::ranges::data(result.bytes_));
            return result;
        }

        template<typename Ch, typename Tr>
        constexpr static uuid from_string(std::basic_string_view<Ch, Tr> const uuid_str) {
            constexpr auto set_byte = [](std::byte &b, Ch const hi, Ch const lo)  {
                constexpr auto value = [](Ch const c) -> std::uint8_t {
                    switch (c) {
                        case '0': return 0;
                        case '1': return 1;
                        case '2': return 2;
                        case '3': return 3;
                        case '4': return 4;
                        case '5': return 5;
                        case '6': return 6;
                        case '7': return 7;
                        case '8': return 8;
                        case '9': return 9;
                        case 'a': [[fallthrough]];
                        case 'A': return 10;
                        case 'b': [[fallthrough]];
                        case 'B': return 11;
                        case 'c': [[fallthrough]];
                        case 'C': return 12;
                        case 'd': [[fallthrough]];
                        case 'D': return 13;
                        case 'e': [[fallthrough]];
                        case 'E': return 14;
                        case 'f': [[fallthrough]];
                        case 'F': return 15;
                        default: throw std::invalid_argument{"Invalid character in UUID string"};
                    }
                };
                b = static_cast<std::byte>(value(hi) << 4 | value(lo));
            };
            [](std::basic_string_view<Ch, Tr> const s) {
                if (36 == s.length()) {
                    if ('-' != s[8] || '-' != s[13] || '-' != s[18] || '-' != s[23]) {
                        throw std::invalid_argument{"Invalid UUID string structure"};
                    }
                }
                else {
                    throw std::invalid_argument{"Invalid UUID string length"};
                }
            }(uuid_str);
            uuid result{};
            set_byte(result.bytes_[0], uuid_str[0], uuid_str[1]);
            set_byte(result.bytes_[1], uuid_str[2], uuid_str[3]);
            set_byte(result.bytes_[2], uuid_str[4], uuid_str[5]);
            set_byte(result.bytes_[3], uuid_str[6], uuid_str[7]);
            set_byte(result.bytes_[4], uuid_str[9], uuid_str[10]);
            set_byte(result.bytes_[5], uuid_str[11], uuid_str[12]);
            set_byte(result.bytes_[6], uuid_str[14], uuid_str[15]);
            set_byte(result.bytes_[7], uuid_str[16], uuid_str[17]);
            set_byte(result.bytes_[8], uuid_str[19], uuid_str[20]);
            set_byte(result.bytes_[9], uuid_str[21], uuid_str[22]);
            set_byte(result.bytes_[10], uuid_str[24], uuid_str[25]);
            set_byte(result.bytes_[11], uuid_str[26], uuid_str[27]);
            set_byte(result.bytes_[12], uuid_str[28], uuid_str[29]);
            set_byte(result.bytes_[13], uuid_str[30], uuid_str[31]);
            set_byte(result.bytes_[14], uuid_str[32], uuid_str[33]);
            set_byte(result.bytes_[15], uuid_str[34], uuid_str[35]);
            return result;
        }

    private:
        static std::mt19937_64 &random_generator() noexcept {
            thread_local std::random_device rd;
            thread_local std::mt19937_64 gen{rd()};
            return gen;
        }

        void set_version(std::uint8_t const version) noexcept {
            bytes_[6] = (bytes_[6] & std::byte{0x0f}) | (std::byte{version} << 4);
        }

        std::array<std::byte, 16> bytes_;
    };
} // skizzay::simple::cqrs

template<>
struct std::hash<skizzay::simple::cqrs::uuid> {
    constexpr std::size_t operator()(skizzay::simple::cqrs::uuid const &uuid) const noexcept {
        std::uint64_t hi = 0;
        std::uint64_t lo = 0;
        std::ranges::copy_n(std::ranges::begin(uuid.bytes_), 8, reinterpret_cast<std::byte *>(&hi));
        std::ranges::copy_n(std::ranges::begin(uuid.bytes_) + 8, 8, reinterpret_cast<std::byte *>(&lo));
        return hi ^ (lo + 0x9e3779b9 + (hi << 6) + (hi >> 2));
    }
};

#endif //UUID_H
