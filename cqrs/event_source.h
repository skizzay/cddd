#ifndef CDDD_CQRS_EVENT_STORE_H__
#define CDDD_CQRS_EVENT_STORE_H__

#include "cqrs/event_stream.h"
#include "cqrs/source.h"


namespace cddd {
namespace cqrs {

typedef source<event, store<event_stream>> event_source;

}
}

#endif
