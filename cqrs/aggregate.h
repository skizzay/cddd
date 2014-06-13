#ifndef CDDD_CQRS_AGGREATE_H__
#define CDDD_CQRS_AGGREATE_H__

#include "cddd/cqrs/object_id.h"
#include "cddd/cqrs/event_dispatcher.h"


namespace cddd {
namespace cqrs {

template<class Alloc>
class basic_aggregate {
public:
   typedef typename basic_event_collection<Alloc>::allocator_type allocator_type;

   virtual ~basic_aggregate() = default;

   inline const object_id & id() const {
      return aggregate_id;
   }

   inline std::size_t revision() const {
      return aggregate_version;
   }

   inline const basic_event_collection<Alloc> &uncommitted_events() const {
      return pending_events;
   }

   inline bool has_uncommitted_events() const {
      return !uncommitted_events().empty();
   }

   inline void clear_uncommitted_events() {
      pending_events.clear();
   }

   inline void apply_change(std::shared_ptr<event> evt) {
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
   basic_aggregate(object_id id_, std::shared_ptr<event_dispatcher> dispatcher_);

   template<class Evt, class Fun>
   inline void add_handler(Fun &&f) {
      event_handler closure = [f=std::forward(f)](const event &e) {
         f(static_cast<const details_::event_wrapper<Evt>&>(e).evt);
      };
      dispatcher->add_handler(typeid(Evt), std::move(closure));
   }

private:
   void apply_change(std::shared_ptr<event> evt, bool is_new) {
      if (evt) {
         dispatcher->dispatch(evt);
         ++aggregate_version;
         if (is_new) {
            pending_events.push_back(evt);
         }
      }
   }

   object_id aggregate_id;
   std::size_t aggregate_version;
   basic_event_collection<Alloc> pending_events;
   std::shared_ptr<event_dispatcher> dispatcher;
};


template<class Alloc> using basic_aggregate_factory = std::function<std::shared_ptr<basic_aggregate<Alloc>>(object_id, std::shared_ptr<event_dispatcher>)>;

}
}

#endif
