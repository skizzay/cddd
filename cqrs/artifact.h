#ifndef CDDD_CQRS_ARTIFACT_H__
#define CDDD_CQRS_ARTIFACT_H__

#include "cddd/cqrs/event_container.h"
#include "cddd/cqrs/event_dispatcher.h"
#include "cddd/cqrs/event_stream.h"


namespace cddd {
namespace cqrs {

class artifact {
public:
   typedef std::size_t size_type;

   artifact() = default;
   artifact(const artifact &) = delete;
   artifact(artifact &&) = default;

   virtual ~artifact() = default;

   artifact & operator =(const artifact &) = delete;
   artifact & operator =(artifact &&) = default;

   virtual event_sequence uncommitted_events() const = 0;

   virtual bool has_uncommitted_events() const = 0;

   virtual size_type size_uncommitted_events() const = 0;

   virtual void clear_uncommitted_events() = 0;

   virtual void load_from_history(const event_stream &stream) = 0;

   virtual void apply_change(event_ptr evt) = 0;

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
};


class basic_artifact : public artifact {
public:
   basic_artifact() = delete;

   inline std::size_t revision() const {
      return artifact_version;
   }

   virtual event_sequence uncommitted_events() const override final {
      return pending_events->events();
   }

   virtual bool has_uncommitted_events() const override final {
      return !pending_events->empty();
   }

   virtual void clear_uncommitted_events() override final {
      pending_events->clear();
   }

   virtual void load_from_history(const event_stream &stream) final override {
      for (auto evt : stream.committed_events()) {
         apply_change(evt, false);
      }
   }

   virtual void apply_change(event_ptr evt) final override {
      apply_change(evt, true);
   }

protected:
   inline basic_artifact(std::shared_ptr<event_dispatcher> dispatcher_,
                         event_container_ptr events_) :
      artifact(),
      artifact_version(0),
      dispatcher(dispatcher_),
      pending_events(events_)
   {
   }

   template<class Evt, class Fun>
   inline void add_handler(Fun &&f) {
      event_handler closure = [f=std::forward(f)](const event &e) {
         f(static_cast<const details_::event_wrapper<Evt>&>(e).evt);
      };
      dispatcher->add_handler(typeid(Evt), std::move(closure));
   }

private:
   inline void apply_change(event_ptr evt, bool is_new) {
      if (evt) {
         dispatcher->dispatch(evt);
         ++artifact_version;
         if (is_new) {
            pending_events->add(evt);
         }
      }
   }

   std::size_t artifact_version;
   std::shared_ptr<event_dispatcher> dispatcher;
   event_container_ptr pending_events;
};


typedef std::shared_ptr<artifact> artifact_ptr;

}
}

#endif
