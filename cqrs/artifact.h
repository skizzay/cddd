#ifndef CDDD_CQRS_ARTIFACT_H__
#define CDDD_CQRS_ARTIFACT_H__

#include <deque>
#include "cqrs/event_dispatcher.h"
#include "cqrs/event_stream.h"


namespace cddd {
namespace cqrs {

template<class EventDispatcher, class EventContainer>
class basic_artifact {
public:
   typedef std::shared_ptr<event> event_ptr;
   typedef EventDispatcher event_dispatcher_type;
   typedef EventContainer event_container_type;
   typedef typename EventContainer::size_type size_type;

   basic_artifact() = delete;

   inline size_type revision() const {
      return artifact_version;
   }

   inline event_sequence uncommitted_events() const {
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

   inline void load_from_history(const event_stream &stream) {
      for (auto evt : stream.load(next_revision())) {
         apply_change(evt, false);
      }
   }

   inline void apply_change(event_ptr evt) {
      apply_change(evt, true);
   }

   template<class Evt>
   inline void apply_change(Evt && e) {
      auto ptr = std::make_shared<details_::event_wrapper<Evt>>(std::forward<Evt>(e), next_revision());
      apply_change(std::static_pointer_cast<event>(ptr));
   }

   template<class EvtAlloc, class Evt>
   inline void apply_change(const EvtAlloc &alloc, Evt && e) {
      auto ptr = std::allocate_shared<details_::event_wrapper<Evt>>(alloc, std::forward<Evt>(e), next_revision());
      apply_change(std::static_pointer_cast<event>(ptr));
   }

protected:
   inline basic_artifact(const event_dispatcher_type &dispatcher_ = event_dispatcher_type{},
                         const event_container_type &events_ = event_container_type{}) :
      artifact_version(0),
      dispatcher(dispatcher_),
      pending_events(events_) {
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
   inline void apply_change(event_ptr evt, bool is_new) {
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
   event_dispatcher_type dispatcher;
   event_container_type pending_events;
};


typedef basic_artifact<event_dispatcher, std::deque<event_ptr>> artifact;

}
}

#endif
