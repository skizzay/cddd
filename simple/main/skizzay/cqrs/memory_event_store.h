//
// Created by andrew on 1/22/24.
//

#ifndef MEMORY_EVENT_STORE_H
#define MEMORY_EVENT_STORE_H

#include "event.h"
#include "uuid.h"
#include <ranges>
#include <unordered_map>
#include <variant>
#include <vector>

namespace skizzay::simple::cqrs {
    template<typename Clock, event... Events>
        requires requires(Clock clock) {
            { clock.now() } -> time_point;
        }
    class memory_event_store {
        class event_stream final {
            friend class memory_event_store;
        public:
            constexpr void put_events(uuid const &id, event_range auto events) {
                store_.put_events(id, std::move(events));
            }

        private:
            constexpr explicit event_stream(memory_event_store &store) noexcept : store_{store} {
            }

            memory_event_store &store_;
        };

        class event_source final {
            friend class memory_event_store;
        public:
            constexpr std::span<std::variant<Events...> const> get_events(uuid const &id, std::uint64_t const begin,
                                                                          std::uint64_t const end
            ) const noexcept {
                return store_.get_events(id, begin, end);
            }

        private:
            constexpr explicit event_source(memory_event_store &store) noexcept : store_{store} {
            }

            memory_event_store &store_;
        };
    public:
        constexpr explicit memory_event_store(Clock &&clock = {}) noexcept(std::is_nothrow_move_constructible_v<Clock>)
            : clock_{std::forward<Clock>(clock)} {
        }

        [[nodiscard]] event_stream as_event_stream() noexcept {
            return event_stream{*this};
        }

        [[nodiscard]] event_source as_event_source() noexcept {
            return event_source{*this};
        }

        [[nodiscard]]
        constexpr std::span<std::variant<Events...> const> get_events(uuid const &id, std::uint64_t const begin,
                                                                      std::uint64_t const end
        ) const noexcept {
            if (auto const it = events_.find(id); it != events_.end()) {
                std::size_t const begin_index = begin - 1;
                if (auto const &events = it->second; begin_index < events.size()) {
                    return std::span{events.data() + begin_index, std::min(end - 1, events.size()) - begin_index};
                }
            }
            return {};
        }

        constexpr void put_events(uuid const &id, event_range auto events) {
            auto &existing_events = events_[id];
            if constexpr (std::ranges::sized_range<decltype(events)>) {
                existing_events.reserve(std::max(existing_events.capacity(),
                                                 std::ranges::size(existing_events) + std::ranges::size(events)));
            }
            std::ranges::copy(
                events | std::views::as_rvalue
                | std::views::transform(update_event{id, clock_.now()}),
                std::back_inserter(existing_events));
        }

    private:
        std::unordered_map<uuid, std::vector<std::variant<Events...> > > events_;
        [[no_unique_address]] Clock clock_;

        struct update_event final {
            template<typename E>
            static constexpr inline bool is_known_event_v = (std::same_as<E, Events> || ...);

            template<event Event>
            requires (std::same_as<Event, Events> || ...)
            constexpr std::variant<Events...> operator()(Event &&e) const noexcept {
                e.stream_id = id;
                e.timestamp = now;
                return e;
            }

            template<event... Es>
            requires (is_known_event_v<Es> && ...)
            constexpr std::variant<Events...> operator()(std::variant<Es...> &&event) const noexcept {
                return std::visit(*this, std::forward<std::variant<Es...>>(event));
            }

            uuid const &id;
            decltype(std::declval<Clock>().now()) const now;
        };
    };
} // skizzay::simple::cqrs

#endif //MEMORY_EVENT_STORE_H
