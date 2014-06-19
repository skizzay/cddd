#ifndef CDDD_CQRS_EVENT_STORE_H__
#define CDDD_CQRS_EVENT_STORE_H__

#include "cddd/cqrs/event_stream.h"


namespace cddd {
namespace cqrs {

class event_store {
public:
   virtual ~event_store() = default;

   virtual event_stream_ptr open_stream(object_id) = 0;
};


typedef std::shared_ptr<event_store> event_store_ptr;

}
}

#endif
