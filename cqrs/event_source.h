#ifndef CDDD_CQRS_EVENT_STORE_H__
#define CDDD_CQRS_EVENT_STORE_H__

#include "cddd/cqrs/event_stream.h"
#include "cddd/cqrs/source.h"


namespace cddd {
namespace cqrs {

typedef source<event, store<event_stream>> event_source;

}
}

#endif
