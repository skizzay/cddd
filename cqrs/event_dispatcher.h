#ifndef CDDD_CQRS_EVENT_DISPATCHER_H__
#define CDDD_CQRS_EVENT_DISPATCHER_H__

#include "cddd/cqrs/event.h"
#include <functional>


namespace cddd {
namespace cqrs {

typedef std::function<void(const event &)> event_handler;


class event_dispatcher {
public:
   virtual ~event_dispatcher() = default;

   virtual void add_handler(std::type_index type, event_handler f) = 0;
   virtual void dispatch(event_ptr evt) = 0;
};

}
}

#endif
