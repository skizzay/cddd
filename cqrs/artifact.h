#ifndef CDDD_CQRS_ARTIFACT_H__
#define CDDD_CQRS_ARTIFACT_H__

#include "cqrs/domain_event.h"
#include "messaging/dispatcher.h"
#include <deque>
#include <sequence.h>
#include <boost/uuid/uuid.hpp>


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


class base_artifact {
public:
   typedef std::shared_ptr<domain_event> domain_event_ptr;
   typedef size_t size_type;
   typedef boost::uuids::uuid id_type;

   virtual const id_type & id() const = 0;
   virtual size_type revision() const = 0;
   virtual domain_event_sequence uncommitted_events() const = 0;
   virtual bool has_uncommitted_events() const = 0;
   virtual size_type size_uncommitted_events() const = 0;
   virtual void clear_uncommitted_events() = 0;
   virtual void load_from_history(domain_event_sequence &events) = 0;
   virtual void apply_change(domain_event_ptr evt) = 0;
};


template<class DomainEventDispatcher, class DomainEventContainer>
class basic_artifact : public base_artifact {
public:
   typedef std::shared_ptr<domain_event> domain_event_ptr;
   typedef DomainEventDispatcher domain_event_dispatcher_type;
   typedef DomainEventContainer domain_event_container_type;
   using base_artifact::size_type;

   virtual const id_type & id() const final override {
      return artifact_id;
   }

   virtual size_type revision() const final override {
      return artifact_version;
   }

   virtual domain_event_sequence uncommitted_events() const final override {
      return sequencing::from(std::begin(pending_events), std::end(pending_events));
   }

   virtual bool has_uncommitted_events() const final override {
      return !pending_events.empty();
   }

   virtual void clear_uncommitted_events() final override {
      pending_events.clear();
   }

   virtual size_type size_uncommitted_events() const final override {
      return pending_events.size();
   }

   virtual void load_from_history(domain_event_sequence &events) final override {
      for (auto evt : std::move(events)) {
         apply_change(evt, false);
      }
   }

   virtual void apply_change(domain_event_ptr evt) final override {
      apply_change(evt, true);
   }

   template<class Evt, class Alloc=std::allocator<Evt>>
   inline void apply_change(Evt && e, const Alloc &alloc={}) {
      const size_type next_revision = this->revision() + this->size_uncommitted_events() + 1;
      auto ptr = std::allocate_shared<basic_domain_event<Evt>>(alloc, std::forward<Evt>(e), next_revision);
      this->apply_change(std::static_pointer_cast<domain_event>(ptr), true);
   }

protected:
   inline basic_artifact(std::shared_ptr<domain_event_dispatcher_type> dispatcher_,
                         const id_type &aid,
                         domain_event_container_type &&events_ = domain_event_container_type{}) :
      artifact_id{aid},
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

   id_type artifact_id;
   size_type artifact_version;
   std::shared_ptr<domain_event_dispatcher_type> dispatcher;
   domain_event_container_type pending_events;
};


typedef basic_artifact<messaging::dispatcher<>, std::deque<domain_event_ptr>> artifact;

}
}

#endif
