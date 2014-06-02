#ifndef CDDD_CQRS_EVENT_DISPATCHER_H__
#define CDDD_CQRS_EVENT_DISPATCHER_H__

#include "cddd/cqrs/event.h"
#include <functional>
#include <memory>
#include <unordered_map>


namespace cddd {
namespace cqrs {

typedef std::function<void(const event &)> event_handler;


class event_dispatcher {
public:
   virtual ~event_dispatcher() = default;

   virtual void add_handler(std::type_index type, event_handler f) = 0;
   virtual void dispatch(std::shared_ptr<event> evt) = 0;
};


template<class Alloc=std::allocator<std::pair<const std::type_index, event_handler>>>
class basic_event_dispatcher : public event_dispatcher {
   typedef std::hash<std::type_index> index_hash;
   typedef std::equal_to<std::type_index> index_equals;
   typedef std::unordered_map<std::type_index, event_handler, index_hash, index_equals, Alloc> dispatch_table;

   dispatch_table handlers;

public:
   typedef typename dispatch_table::allocator_type allocator_type;

   basic_event_dispatcher() = default;
   basic_event_dispatcher(const basic_event_dispatcher &) = delete;
   basic_event_dispatcher(basic_event_dispatcher &&) = default;
   explicit inline basic_event_dispatcher(const allocator_type &alloc) :
      handlers(alloc)
   {
   }

   virtual ~basic_event_dispatcher() = default;

   basic_event_dispatcher &operator =(const basic_event_dispatcher &) = delete;
   basic_event_dispatcher &operator =(basic_event_dispatcher &&) = default;

   virtual void add_handler(std::type_index type, event_handler f) {
      handlers.emplace(type, std::move(f));
   }

   virtual void dispatch(std::shared_ptr<event> evt) {
      if (evt) {
         auto handler = handlers.at(evt->type());
         handler(evt);
      }
   }
};


}
}

#endif
