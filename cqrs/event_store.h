#ifndef CDDD_CQRS_EVENT_STORE_H__
#define CDDD_CQRS_EVENT_STORE_H__

#include "cddd/cqrs/object_id.h"
#include "cddd/cqrs/event.h"


namespace cddd {
namespace cqrs {

class event_store {
public:
   virtual ~event_store() = default;
};

}
}

#endif

