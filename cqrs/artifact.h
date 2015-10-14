#pragma once

#include "cqrs/domain_event.h"
#include "messaging/dispatcher.h"
#include "utils/validation.h"
#include <deque>


namespace cddd {
namespace cqrs {

namespace details_ {

template<size_t I> struct int_to_type {};
template<class T> struct argument_to_type {};

template<class Fun, class EventType>
auto create_handler(Fun f, int_to_type<1>, argument_to_type<EventType>, std::true_type) {
   return [f](const domain_event &event) {
         const EventType &e = static_cast<const EventType &>(event);
         return f(e);
      };
}

template<class Fun, class EventType>
auto create_handler(Fun f, int_to_type<1>, argument_to_type<EventType>, std::false_type) {
   return [f](const domain_event &event) {
         const basic_domain_event<EventType> &e = static_cast<const basic_domain_event<EventType> &>(event);
         return f(e.event());
      };
}

template<class Fun, class EventType>
auto create_handler(Fun f, int_to_type<1>, argument_to_type<basic_domain_event<EventType>>, std::true_type) {
   return [f](const domain_event &event) {
         const basic_domain_event<EventType> &e = static_cast<const basic_domain_event<EventType> &>(event);
         return f(e.event());
      };
}

template<class Fun, class EventType>
auto create_handler(Fun f, int_to_type<2>, argument_to_type<EventType>, std::true_type) {
   static_assert(std::is_convertible<size_t, typename utils::function_traits<Fun>::template argument<1>::type>::value,
                 "Handler's second argument must be size_t to get version.");

   return [f](const domain_event &event) {
         const EventType &e = static_cast<const EventType &>(event);
         return f(e, e.version());
      };
}

template<class Fun, class EventType>
auto create_handler(Fun f, int_to_type<2>, argument_to_type<EventType>, std::false_type) {
   static_assert(std::is_convertible<size_t, typename utils::function_traits<Fun>::template argument<1>::type>::value,
                 "Handler's second argument must be size_t to get version.");

   return [f](const domain_event &event) {
         const basic_domain_event<EventType> &e = static_cast<const basic_domain_event<EventType> &>(event);
         return f(e.event(), e.version());
      };
}

template<class Fun, class EventType>
auto create_handler(Fun f, int_to_type<2>, argument_to_type<basic_domain_event<EventType>>, std::true_type) {
   static_assert(std::is_convertible<size_t, typename utils::function_traits<Fun>::template argument<1>::type>::value,
                 "Handler's second argument must be size_t to get version.");

   return [f](const domain_event &event) {
         const basic_domain_event<EventType> &e = static_cast<const basic_domain_event<EventType> &>(event);
         return f(e.event(), e.version());
      };
}

}


template<class DomainEventDispatcher, class DomainEventContainer>
class basic_artifact {
   template<class> friend class basic_artifact_view;

public:
   typedef boost::uuids::uuid id_type;
   typedef DomainEventDispatcher domain_event_dispatcher_type;
   typedef DomainEventContainer domain_event_container_type;
   using size_type = typename domain_event_container_type::size_type;

   const id_type & id() const {
      return artifact_id;
   }

   size_type revision() const {
      return artifact_version;
   }

   const auto &uncommitted_events() const {
      using std::begin;
      using std::end;

      return pending_events;
   }

   bool has_uncommitted_events() const {
      return !pending_events.empty();
   }

   void clear_uncommitted_events() {
      pending_events.clear();
   }

   size_type size_uncommitted_events() const {
      return pending_events.size();
   }

   template<class EventsContainer>
   void load_from_history(EventsContainer &&events) {
      using std::move;

      for (auto evt : move(events)) {
         apply_change(evt, false);
      }
   }

   template<class Evt>
   inline void apply_change(Evt && e) {
      using std::allocate_shared;
      using std::forward;
      using std::static_pointer_cast;
      using allocator_type = typename domain_event_container_type::allocator_type::template rebind<basic_domain_event<Evt>>::other;

      allocator_type allocator{pending_events.get_allocator()};
      size_type next_revision = revision() + size_uncommitted_events() + 1;
      auto ptr = allocate_shared<basic_domain_event<Evt>>(allocator, forward<Evt>(e), next_revision);
      apply_change(static_pointer_cast<domain_event>(ptr), true);
   }

protected:
   inline basic_artifact(const id_type &aid) :
      artifact_id(aid),
      artifact_version(0),
      dispatcher{},
      pending_events{}
   {
      utils::validate_id(id());
   }

   basic_artifact(basic_artifact &&) = default;
   basic_artifact &operator =(basic_artifact &&) = default;

   basic_artifact() = delete;
   basic_artifact(const basic_artifact &) = delete;
   basic_artifact &operator =(const basic_artifact &) = delete;

   template<class Fun>
   void add_handler(Fun f) {
      using std::move;
      typedef messaging::message_from_argument<Fun> event_type;
      typedef details_::int_to_type<utils::function_traits<Fun>::arity> num_arguments;
      typedef details_::argument_to_type<event_type> argument_type;

      auto handler = details_::create_handler(move(f), num_arguments{}, argument_type{},
                                              std::is_base_of<domain_event, event_type>{});
      dispatcher.add_message_handler(handler, [](const domain_event &event) {
            return event.type() == utils::type_id_generator::get_id_for_type<event_type>();
         });
   }

   inline void set_version(size_type version) {
      artifact_version = version;
   }

private:
   template<class DomainEventPointer>
   inline void apply_change(DomainEventPointer evt, bool is_new) {
      if (evt != nullptr) {
         // We need to append to the pending events so that our next apply_change
         // call will have an accurate revision.  We do this by placing a null pointer
         // as a placeholder.  If the dispatch fails, we need to discard this event
         // because it was not properly handled.  If it succeeds, then we replace
         // the placeholder with the actual event.
         if (is_new) {
            pending_events.emplace_back(nullptr);
         }
         try {
            dispatcher.dispatch_message(*evt);
            pending_events.back() = std::move(evt);
         }
         catch (...) {
            pending_events.pop_back();
            throw;
         }
      }
   }

   id_type artifact_id;
   size_type artifact_version;
   domain_event_dispatcher_type dispatcher;
   domain_event_container_type pending_events;
};


typedef basic_artifact<messaging::dispatcher<>, std::deque<std::shared_ptr<domain_event>>> artifact;

}
}
