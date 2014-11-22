#ifndef CDDD_CQRS_ARTIFACT_H__
#define CDDD_CQRS_ARTIFACT_H__

#include "cqrs/event_dispatcher.h"
#include <deque>


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
                         domain_event_dispatcher_type &&dispatcher_ = domain_event_dispatcher_type{},
                         domain_event_container_type &&events_ = domain_event_container_type{}) :
      artifact_version(revision_),
      dispatcher(std::forward<domain_event_dispatcher_type>(dispatcher_)),
      pending_events(std::forward<domain_event_container_type>(events_)) {
   }

   basic_artifact(basic_artifact &&) = default;
   basic_artifact &operator =(basic_artifact &&) = default;

   basic_artifact(const basic_artifact &) = delete;
   basic_artifact &operator =(const basic_artifact &) = delete;

   template<class Evt, class Fun>
   inline void add_handler(Fun &&f) {
      dispatcher.add_handler<Evt>(std::forward<Fun>(f));
   }

private:
   inline void apply_change(domain_event_ptr evt, bool is_new) {
      if (evt != nullptr) {
         if (is_new) {
            pending_events.push_back(evt);
         }
         else {
            dispatcher.dispatch(*evt);
         }
      }
   }

   inline size_type next_revision() const {
      return revision() + size_uncommitted_events() + 1;
   }

   size_type artifact_version;
   domain_event_dispatcher_type dispatcher;
   domain_event_container_type pending_events;
};


typedef basic_artifact<domain_event_dispatcher, std::deque<domain_event_ptr>> artifact;

}
}

#endif
