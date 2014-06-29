#ifndef CDDD_CQRS_DEFAULT_EVENT_DISPATCHER_H__
#define CDDD_CQRS_DEFAULT_EVENT_DISPATCHER_H__

#include "cddd/cqrs/event.h"
#include <functional>
#include <unordered_map>


namespace cddd {
namespace cqrs {

typedef std::function<void(const event &)> event_handler;


template<class Alloc=std::allocator<std::pair<const std::type_index, event_handler>>>
class basic_event_dispatcher {
   typedef std::hash<std::type_index> index_hash;
   typedef std::equal_to<std::type_index> index_equals;
   typedef std::unordered_map<std::type_index, event_handler, index_hash, index_equals, Alloc> dispatch_table;

   dispatch_table handlers;

public:
   typedef typename dispatch_table::allocator_type allocator_type;

   basic_event_dispatcher(const basic_event_dispatcher &) = delete;
   basic_event_dispatcher(basic_event_dispatcher &&) = default;
   explicit inline basic_event_dispatcher(const allocator_type &alloc=allocator_type()) :
      handlers(alloc)
   {
   }

   basic_event_dispatcher &operator =(const basic_event_dispatcher &) = delete;
   basic_event_dispatcher &operator =(basic_event_dispatcher &&) = default;

   template<class Evt, class Fun>
   inline void add_handler(Fun f) {
      event_handler closure = [f=std::move(f)](const event &e) {
         f(static_cast<const details_::event_wrapper<Evt>&>(e).evt);
      };
      add_handler(typeid(Evt), std::move(closure));
   }

   inline void add_handler(std::type_index type, event_handler f) {
      handlers.emplace(type, std::move(f));
   }

   inline bool has_handler(std::type_index type) const {
      return handlers.find(type) != handlers.end();
   }

   template<class Evt>
   inline bool has_handler() const {
      return has_handler(typeid(Evt));
   }

   inline void dispatch(const event &e) {
      auto &handler = handlers.at(e.type());
      handler(e);
   }
};


typedef basic_event_dispatcher<> event_dispatcher;


}
}

#endif

