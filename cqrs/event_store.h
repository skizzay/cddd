#ifndef CDDD_CQRS_EVENT_STORE_H__
#define CDDD_CQRS_EVENT_STORE_H__

#include "cddd/cqrs/event.h"
#include "cddd/cqrs/store.h"


namespace cddd {
namespace cqrs {

typedef store<event> event_store;
typedef std::shared_ptr<event_store> event_store_ptr;

}
}

#endif
