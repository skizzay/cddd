#ifndef CDDD_CQRS_AGGREATE_H__
#define CDDD_CQRS_AGGREATE_H__

#include "cddd/cqrs/object_id.h"
#include "cddd/cqrs/event_dispatcher.h"


namespace cddd {
namespace cqrs {

class aggregate {
public:
   virtual ~aggregate() = default;

   virtual const object_id & id() const = 0;
   virtual std::size_t revision() const = 0;
   virtual const event_collection &uncommitted_events() const = 0;
   virtual bool has_uncommitted_events() const = 0;
   virtual void clear_uncommitted_events() = 0;
   virtual void apply_change(std::shared_ptr<event> evt) = 0;

   template<class Evt>
   inline void apply_change(Evt &&e) {
      auto ptr = std::make_shared<details_::event_wrapper<Evt>>(std::forward(e));
      apply_change(std::static_pointer_cast<event>(ptr));
   }
   template<class Alloc, class Evt>
   inline void apply_change(const Alloc &alloc, Evt &&e) {
      auto ptr = std::allocate_shared<details_::event_wrapper<Evt>>(alloc, std::forward(e));
      apply_change(std::static_pointer_cast<event>(ptr));
   }
};


class basic_aggregate : public aggregate {
public:
   virtual ~basic_aggregate() = default;

   virtual const object_id & id() const final;
   virtual std::size_t revision() const final;
   virtual const event_collection &uncommitted_events() const final;
   virtual bool has_uncommitted_events() const final;
   virtual void clear_uncommitted_events() final;
   virtual void apply_change(std::shared_ptr<event> evt) final;

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
   void apply_change(std::shared_ptr<event> evt, bool is_new);

   object_id aggregate_id;
   std::size_t aggregate_version;
   event_collection pending_events;
   std::shared_ptr<event_dispatcher> dispatcher;
};


typedef std::function<std::shared_ptr<aggregate>(object_id, std::shared_ptr<event_dispatcher>)> aggregate_factory;

}
}

#endif
