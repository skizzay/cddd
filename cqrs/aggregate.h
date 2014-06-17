#ifndef CDDD_CQRS_AGGREATE_H__
#define CDDD_CQRS_AGGREATE_H__

#include "cddd/cqrs/artifact.h"


namespace cddd {
namespace cqrs {

template<class Alloc=event_collection::allocator_type>
class basic_aggregate : public basic_artifact<Alloc> {
public:
   typedef typename basic_event_collection<Alloc>::allocator_type allocator_type;

   basic_aggregate() = default;
   basic_aggregate(const basic_aggregate &) = delete;
   basic_aggregate(basic_aggregate &&) = default;

   virtual ~basic_aggregate() = default;

   basic_aggregate & operator =(const basic_aggregate &) = delete;
   basic_aggregate & operator =(basic_aggregate &&) = default;

   inline const object_id & id() const {
      return aggregate_id;
   }

protected:
   inline basic_aggregate(object_id id_, std::shared_ptr<event_dispatcher> dispatcher_, allocator_type alloc=allocator_type()) :
      basic_artifact<Alloc>(dispatcher_),
      aggregate_id(id_),
      aggregate_version(0),
      pending_events(alloc),
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

private:
   void apply_change(event_ptr evt, bool is_new) {
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
