#pragma once

#include "skizzay/cddd/aggregate_root.h"
#include "skizzay/cddd/dereference.h"
#include "skizzay/cddd/event_sourced.h"
#include "skizzay/cddd/nullable.h"
#include "skizzay/cddd/repository.h"

namespace skizzay::cddd::in_memory {namespace event_source_details_ {
    template<typename Store>
    struct impl {
      friend Store;

      template<concepts::aggregate_root AggregateRoot>
      constexpr void load_from_history(AggregateRoot &aggregate_root,
                                       version_t<AggregateRoot> const target_version
      ) const {
        using buffer_type = std::shared_ptr<std::remove_cvref_t<dereference_t<
          decltype(cddd::get(std::declval<Store const>().event_buffers(),
                             cddd::id(aggregate_root)))> > >;

        if (auto event_buffer = cddd::get(
            store_->event_buffers(), cddd::id(aggregate_root),
            null_factory<buffer_type>{});
          !cddd::is_null(event_buffer)) {
          this->load_from_history(
            aggregate_root,
            dereference(event_buffer)
            .get_events(cddd::version(aggregate_root) + 1,
                        target_version));
        }
      }

      constexpr bool contains(concepts::identifier auto const &id_value) const noexcept {
        return cddd::contains(store_->event_buffers(), id_value);
      }

    private:
      constexpr explicit impl(Store const &store) noexcept
        : store_{&store} {
      }

      Store const *store_;

      constexpr void load_from_history(auto &aggregate_root,
                                       std::ranges::sized_range auto const &events
      ) const {
        for (auto const &event: events) {
          cddd::apply_event(dereference(aggregate_root),
                            dereference(event));
        }
      }
    };
  } // namespace event_source_details_

  template<typename Store>
  // requires concepts::domain_event_sequence<typename
  // Store::domain_event_sequence>
  using event_source = event_source_details_::impl<Store>;
} // namespace skizzay::cddd::in_memory
