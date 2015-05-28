#ifndef CDDD_CQRS_ARTIFACT_H__
#define CDDD_CQRS_ARTIFACT_H__

#include "cqrs/domain_event.h"
#include "messaging/dispatcher.h"
#include <deque>
#include <sequence.h>


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
   static_assert(std::is_convertible<size_t, typename utils::function_traits<Fun>::template argument_at<1>::type>::value,
                 "Handler's second argument must be size_t to get version.");

   return [f](const domain_event &event) {
         const EventType &e = static_cast<const EventType &>(event);
         return f(e, e.version());
      };
}

template<class Fun, class EventType>
auto create_handler(Fun f, int_to_type<2>, argument_to_type<EventType>, std::false_type) {
   static_assert(std::is_convertible<size_t, typename utils::function_traits<Fun>::template argument_at<1>::type>::value,
                 "Handler's second argument must be size_t to get version.");

   return [f](const domain_event &event) {
         const basic_domain_event<EventType> &e = static_cast<const basic_domain_event<EventType> &>(event);
         return f(e.event(), e.version());
      };
}

template<class Fun, class EventType>
auto create_handler(Fun f, int_to_type<2>, argument_to_type<basic_domain_event<EventType>>, std::true_type) {
   static_assert(std::is_convertible<size_t, typename utils::function_traits<Fun>::template argument_at<1>::type>::value,
                 "Handler's second argument must be size_t to get version.");

   return [f](const domain_event &event) {
         const basic_domain_event<EventType> &e = static_cast<const basic_domain_event<EventType> &>(event);
         return f(e.event(), e.version());
      };
}

}

template<class DomainEventDispatcher, class DomainEventContainer>
class basic_artifact {
public:
   typedef std::shared_ptr<domain_event> domain_event_ptr;
   typedef DomainEventDispatcher domain_event_dispatcher_type;
   typedef DomainEventContainer domain_event_container_type;
   typedef typename DomainEventContainer::size_type size_type;

   inline size_type revision() const {
      return artifact_version;
   }

   inline domain_event_sequence uncommitted_events() const {
      return sequencing::from(std::begin(pending_events), std::end(pending_events));
   }

   inline bool has_uncommitted_events() const {
      return !pending_events.empty();
   }

   inline void clear_uncommitted_events() {
      pending_events.clear();
   }

   inline size_type size_uncommitted_events() const {
      return pending_events.size();
   }

   template<class DomainEventStream>
   inline void load_from_history(const DomainEventStream &stream) {
      for (auto evt : stream.load(next_revision())) {
         apply_change(evt, false);
      }
   }

   inline void apply_change(domain_event_ptr evt) {
      apply_change(evt, true);
   }

   template<class Evt, class Alloc=std::allocator<Evt>>
   inline void apply_change(Evt && e, const Alloc &alloc={}) {
      auto ptr = std::allocate_shared<basic_domain_event<Evt>>(alloc, std::forward<Evt>(e), next_revision());
      apply_change(std::static_pointer_cast<domain_event>(ptr));
   }

protected:
   inline basic_artifact(std::shared_ptr<domain_event_dispatcher_type> dispatcher_,
                         domain_event_container_type &&events_ = domain_event_container_type{}) :
      artifact_version(0),
      dispatcher(dispatcher_),
      pending_events(std::forward<domain_event_container_type>(events_)) {
   }

   inline basic_artifact() :
      basic_artifact{std::make_shared<domain_event_dispatcher_type>()}
   {
   }

   basic_artifact(basic_artifact &&) = default;
   basic_artifact &operator =(basic_artifact &&) = default;

   basic_artifact(const basic_artifact &) = delete;
   basic_artifact &operator =(const basic_artifact &) = delete;

   template<class Fun>
   void add_handler(Fun f) {
      typedef messaging::message_from_argument<Fun> event_type;
      typedef details_::int_to_type<utils::function_traits<Fun>::arity> num_arguments;
      typedef details_::argument_to_type<event_type> argument_type;

      auto handler = details_::create_handler(std::move(f), num_arguments{}, argument_type{},
                                              std::is_base_of<domain_event, event_type>{});
      dispatcher->add_message_handler(handler, [](const domain_event &event) {
            return event.type() == utils::type_id_generator::get_id_for_type<event_type>();
         });
   }

   inline void set_version(size_type version) {
      artifact_version = version;
   }

private:
   inline void apply_change(domain_event_ptr evt, bool is_new) {
      if (evt != nullptr) {
         if (is_new) {
            pending_events.push_back(evt);
         }
         dispatcher->dispatch_message(*evt);
      }
   }

   inline size_type next_revision() const {
      return revision() + size_uncommitted_events() + 1;
   }

   size_type artifact_version;
   std::shared_ptr<domain_event_dispatcher_type> dispatcher;
   domain_event_container_type pending_events;
};


typedef basic_artifact<messaging::dispatcher<>, std::deque<domain_event_ptr>> artifact;

}
}

#endif
