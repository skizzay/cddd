#ifndef CDDD_CQRS_EVENT_STREAM_H__
#define CDDD_CQRS_EVENT_STREAM_H__

#include "cqrs/stream.h"
#include "cqrs/event.h"
#include <limits>


namespace cddd {
namespace cqrs {

typedef stream<std::shared_ptr<event>> event_stream;

}
}

#endif
