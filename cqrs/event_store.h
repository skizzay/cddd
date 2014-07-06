#ifndef CDDD_CQRS_EVENT_STORE_H__
#define CDDD_CQRS_EVENT_STORE_H__

#include "cddd/cqrs/event.h"
#include "cddd/cqrs/store.h"


namespace cddd {
namespace cqrs {

#if 0
class event_store {
public:
   virtual ~event_store() = default;

   virtual event_stream_ptr open_stream(object_id) = 0;
};
#endif

typedef store<event> event_store;
typedef std::shared_ptr<event_store> event_store_ptr;

}
}

#endif
