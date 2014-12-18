#ifndef CDDD_CQRS_ARTIFACT_H__
#define CDDD_CQRS_ARTIFACT_H__

#include "cqrs/domain_event.h"
#include "messaging/dispatcher.h"
#include <deque>
#include <sequence.h>


namespace cddd {
namespace cqrs {

template<class DomainEventDispatcher, class DomainEventContainer>
class basic_artifact {
public:
   typedef std::shared_ptr<domain_event> domain_event_ptr;
   typedef DomainEventDispatcher domain_event_dispatcher_type;
   typedef DomainEventContainer domain_event_container_type;
   typedef typename DomainEventContainer::size_type size_type;

   basic_artifact() = delete;

   inline size_type revision() const {
      return artifact_version;
   }

   inline domain_event_sequence uncommitted_events() const {
      return std::experimental::from(std::begin(pending_events), std::end(pending_events));
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

   template<class Evt>
   inline void apply_change(Evt && e) {
      auto ptr = std::make_shared<details_::domain_event_wrapper<Evt>>(std::forward<Evt>(e), next_revision());
      apply_change(std::static_pointer_cast<domain_event>(ptr));
   }

   template<class EvtAlloc, class Evt>
   inline void apply_change(const EvtAlloc &alloc, Evt && e) {
      auto ptr = std::allocate_shared<details_::domain_event_wrapper<Evt>>(alloc, std::forward<Evt>(e), next_revision());
      apply_change(std::static_pointer_cast<domain_event>(ptr));
   }

protected:
   inline basic_artifact(size_type revision_ = 0,
                         std::shared_ptr<domain_event_dispatcher_type> dispatcher_ = std::make_shared<domain_event_dispatcher_type>(),
                         domain_event_container_type &&events_ = domain_event_container_type{}) :
      artifact_version(revision_),
      dispatcher(dispatcher_),
      pending_events(std::forward<domain_event_container_type>(events_)) {
   }

   basic_artifact(basic_artifact &&) = default;
   basic_artifact &operator =(basic_artifact &&) = default;

   basic_artifact(const basic_artifact &) = delete;
   basic_artifact &operator =(const basic_artifact &) = delete;

   template<class Fun>
   inline void add_handler(Fun f) {
      typedef messaging::message_from_argument<Fun> event_type;
      static_assert(std::is_base_of<domain_event, event_type>{}(),
                    "Handler must handle domain events.");
      dispatcher->add_message_handler(std::move(f));
   }

private:
   inline void apply_change(domain_event_ptr evt, bool is_new) {
      if (evt != nullptr) {
         if (is_new) {
            pending_events.push_back(evt);
         }
         else {
            dispatcher->dispatch_message(*evt);
         }
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
