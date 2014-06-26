#ifndef CDDD_CQRS_EVENT_CONTAINER_H__
#define CDDD_CQRS_EVENT_CONTAINER_H__

#include "cddd/cqrs/event.h"

namespace cddd {
namespace cqrs {

class event_container {
public:
   virtual ~event_container() = default;

   virtual void add(event_ptr) = 0;
   virtual void clear() = 0;

   virtual event_sequence events() const = 0;
   virtual std::size_t size() const = 0;
   virtual bool empty() const = 0;
};


typedef std::shared_ptr<event_container> event_container_ptr;

}
}

#endif
