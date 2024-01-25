//
// Created by andrew on 1/22/24.
//

#ifndef AGGREGATE_ROOT_FACTORY_H
#define AGGREGATE_ROOT_FACTORY_H
#include <utility>
#include "event_source.h"
#include "aggregate_root.h"
#include "factory.h"

namespace skizzay::simple::cqrs {
    class uuid;

    template<typename EventSource, typename AggregateRoot=void, typename Factory=default_factory<AggregateRoot>>
    class aggregate_root_factory {
    public:
        explicit aggregate_root_factory(EventSource &&event_source, Factory &&factory = {}
        ) noexcept(std::is_nothrow_move_constructible_v<EventSource> && std::is_nothrow_move_constructible_v<Factory>)
            : event_source_{std::forward<EventSource>(event_source)},
              factory_{std::forward<Factory>(factory)} {
        }

        template<typename AR=AggregateRoot>
        constexpr AR create(uuid const &id) {
            AR entity{[this](uuid const &aggregate_id) -> AR {
                if constexpr (factory<Factory, AR, uuid const &>) {
                    return factory_(aggregate_id);
                }
                else {
                    return factory_.template operator()<AR>(aggregate_id);
                }
            }(id)};
            entity.restore_from(get_events(event_source_, id, entity.version() + 1));
            return entity;
        }

    private:
        EventSource event_source_;
        [[no_unique_address]] Factory factory_;
    };

    template<typename EventSource, typename Factory>
    aggregate_root_factory(EventSource &&, Factory &&) -> aggregate_root_factory<EventSource, std::invoke_result_t<Factory, uuid const &>, Factory>;
} // skizzay::simple::cqrs

#endif //AGGREGATE_ROOT_FACTORY_H
