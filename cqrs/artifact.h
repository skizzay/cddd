#ifndef CDDD_CQRS_ARTIFACT_H__
#define CDDD_CQRS_ARTIFACT_H__

#include "cddd/cqrs/event_dispatcher.h"
#include "cddd/cqrs/event_stream.h"


namespace cddd {
namespace cqrs {

class artifact {
public:
   artifact() = delete;
   artifact(const artifact &) = delete;
   artifact(artifact &&) = default;

   virtual ~artifact() = default;

   artifact & operator =(const artifact &) = delete;
   artifact & operator =(artifact &&) = default;

   inline std::size_t revision() const {
      return artifact_version;
   }

   virtual event_sequence uncommitted_events() const = 0;

   virtual bool has_uncommitted_events() const = 0;

   virtual void clear_uncommitted_events() = 0;

   inline void load_from_history(const event_stream &stream) {
      for (auto evt : stream.committed_events()) {
         apply_change(evt, false);
      }
   }

   inline void apply_change(event_ptr evt) {
      apply_change(evt, true);
   }

   template<class Evt>
   inline void apply_change(Evt &&e) {
      auto ptr = std::make_shared<details_::event_wrapper<Evt>>(std::forward(e));
      apply_change(std::static_pointer_cast<event>(ptr));
   }

   template<class EvtAlloc, class Evt>
   inline void apply_change(const EvtAlloc &alloc, Evt &&e) {
      auto ptr = std::allocate_shared<details_::event_wrapper<Evt>>(alloc, std::forward(e));
      apply_change(std::static_pointer_cast<event>(ptr));
   }

protected:
   inline artifact(std::shared_ptr<event_dispatcher> dispatcher_) :
      artifact_version(0),
      dispatcher(dispatcher_)
   {
   }

   template<class Evt, class Fun>
   inline void add_handler(Fun &&f) {
      event_handler closure = [f=std::forward(f)](const event &e) {
         f(static_cast<const details_::event_wrapper<Evt>&>(e).evt);
      };
      dispatcher->add_handler(typeid(Evt), std::move(closure));
   }

   virtual void add_pending_event(event_ptr evt) = 0;

private:
   inline void apply_change(event_ptr evt, bool is_new) {
      if (evt) {
         dispatcher->dispatch(evt);
         ++artifact_version;
         if (is_new) {
            add_pending_event(evt);
         }
      }
   }

   std::size_t artifact_version;
   std::shared_ptr<event_dispatcher> dispatcher;
};


typedef std::shared_ptr<artifact> artifact_ptr;

}
}

#endif
