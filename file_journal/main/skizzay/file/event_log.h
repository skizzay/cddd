//
// Created by andrew on 2/12/24.
//

#pragma once

#include <cstddef>
#include <skizzay/cqrs/event.h>
#include <skizzay/s11n/binary_reader.h>
#include <skizzay/s11n/io_device.h>
#include <skizzay/s11n/memory_buffer.h>
#include <span>

#include "skizzay/s11n/binary_writer.h"
#include "skizzay/s11n/write_transaction.h"

namespace skizzay::simple::cqrs::file_journal {
    template<typename, std::endian=std::endian::native>
    class event_log;

    namespace file_journal_details_ {

        template<std::endian E>
        class event_log_iterator final {
            template<typename T>
            friend event_log_iterator event_log<T, E>::begin();
            template<typename T>
            friend event_log_iterator event_log<T, E>::end();

            static constexpr std::size_t index_block_size = 2 * sizeof(std::uint64_t);

        public:
            using element_type = std::span<std::byte const>;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::random_access_iterator_tag;

            event_log_iterator() noexcept = default;

            element_type operator*() const noexcept {
                return element_;
            }

            element_type const *operator->() const noexcept {
                return &element_;
            }

            event_log_iterator &operator++() {
                ++element_index_;
                read_next_element();
                return *this;
            }

            event_log_iterator operator++(int) {
                event_log_iterator result{*this};
                ++*this;
                return result;
            }

            event_log_iterator &operator--() {
                --element_index_;
                read_next_element();
                return *this;
            }

            event_log_iterator operator--(int) {
                event_log_iterator result{*this};
                --*this;
                return result;
            }

            event_log_iterator &operator +=(difference_type const n) {
                if (0 != n) {
                    element_index_ += n;
                    read_next_element();
                }
                return *this;
            }

            friend event_log_iterator operator+(event_log_iterator lhs, difference_type const n) {
                lhs += n;
                return lhs;
            }

            friend event_log_iterator operator+(difference_type const n, event_log_iterator const &rhs) {
                return rhs + n;
            }

            event_log_iterator &operator -=(difference_type const n) {
                return *this += -n;
            }

            friend event_log_iterator operator-(event_log_iterator lhs, difference_type const n) {
                lhs -= n;
                return lhs;
            }

            friend difference_type operator-(event_log_iterator const &lhs, event_log_iterator const &rhs) {
                return lhs.element_index_ - rhs.element_index_;
            }

            element_type operator[](difference_type const n) const {
                return *(*this + n);
            }

            friend auto operator<=>(event_log_iterator const &lhs, event_log_iterator const &rhs) {
                return lhs.element_.data() <=> rhs.element_.data();
            }

            friend bool operator==(event_log_iterator const &lhs, event_log_iterator const &rhs) noexcept {
                return lhs.element_.data() == rhs.element_.data();
            }

            friend bool operator!=(event_log_iterator const &lhs, event_log_iterator const &rhs) noexcept {
                return !(lhs == rhs);
            }

            friend bool operator==(event_log_iterator const &lhs, std::default_sentinel_t) noexcept {
                return lhs.element_.data() == nullptr;
            }

            friend bool operator==(std::default_sentinel_t, event_log_iterator const &rhs) noexcept {
                return nullptr == rhs.element_.data();
            }

            friend bool operator!=(event_log_iterator const &lhs, std::default_sentinel_t) noexcept {
                return !(lhs == std::default_sentinel);
            }

            friend bool operator!=(std::default_sentinel_t, event_log_iterator const &rhs) noexcept {
                return !(std::default_sentinel == rhs);
            }

        private:
            explicit event_log_iterator(element_type const log_buffer, element_type const index_buffer, s11n::seek_origin const origin
            ) noexcept
                : log_buffer_{log_buffer},
                  index_buffer_{index_buffer},
                  element_index_{
                      s11n::seek_origin::end == origin
                          ? static_cast<std::ptrdiff_t>(index_buffer_.size() / index_block_size)
                          : 0
                  } {
                read_next_element();
            }

            void read_next_element() {
                auto const [position, size] = [this] {
                    auto const element_index_offset = element_index_ * index_block_size;
                    if (index_buffer_.size() == element_index_offset) {
                        return std::pair{log_buffer_.size(), static_cast<std::uint64_t>(0)};
                    }
                    s11n::buffer_source index{index_buffer_.subspan(element_index_offset, index_block_size)};
                    s11n::binary_reader<s11n::buffer_source, E> reader{index};
                    std::uint64_t p{};
                    std::uint64_t n{};
                    reader >> p >> n;
                    return std::pair{p, n};
                }();
                element_ = log_buffer_.subspan(position, size);
            }

            element_type log_buffer_ = {};
            element_type index_buffer_ = {};
            std::ptrdiff_t element_index_ = 0;
            element_type element_ = {};
        };

    }


    template<s11n::random_access_source Device, std::endian E>
        requires s11n::random_access_sink<Device>
                 && s11n::has_independent_read_write_pointers_v<Device>
                 && requires(Device const &device) {
                     { device.read_region() } noexcept -> std::same_as<std::span<std::byte const> >;
                 }
    class event_log<Device, E> : public std::ranges::view_interface<event_log<Device, E> > {
    public:
        using value_type = std::span<std::byte const>;
        using iterator = file_journal_details_::event_log_iterator<E>;

        explicit event_log(Device log, Device index) noexcept
            : log_{std::move(log)},
              index_{std::move(index)} {
            s11n::seek_write(log_, 0, s11n::seek_origin::end);
            s11n::seek_write(index_, 0, s11n::seek_origin::end);
        }

        event_log(event_log const &) = delete;
        event_log(event_log &&) noexcept = default;
        event_log &operator=(event_log const &) = delete;
        event_log &operator=(event_log &&) noexcept = default;

        ~event_log() noexcept {
            if constexpr (s11n::io_device<Device>) {
                if (s11n::is_open(index_)) {
                    s11n::truncate(index_, s11n::write_position(index_));
                }
                if (s11n::is_open(log_)) {
                    s11n::truncate(log_, s11n::write_position(log_));
                }
            }
        }

        auto begin_transaction() {
            return s11n::write_transaction{log_, index_};
        }

        iterator begin() const {
            return iterator{log_.read_region(), index_.read_region(), s11n::seek_origin::beginning};
        }

        iterator end() const {
            return iterator{log_.read_region(), index_.read_region(), s11n::seek_origin::end};
        }

        void push(event auto const &e) {
            auto const position = log_position();
            s11n::binary_writer<Device, E> log_writer{log_};
            log_writer << e;
            s11n::binary_writer<Device, E> index_writer{index_};
            index_writer << position << (log_position() - position);
        }

    private:
        Device log_;
        Device index_;

        auto log_position() const {
            return s11n::write_position(log_);
        }
    };

    template<typename T>
    event_log(T &&, T &&) -> event_log<T>;

    template<typename Device>
    using big_endian_event_log = event_log<Device, std::endian::big>;

    template<typename Device>
    using little_endian_event_log = event_log<Device, std::endian::little>;

    template<typename Device>
    using native_endian_event_log = event_log<Device, std::endian::native>;
} // skizzay
