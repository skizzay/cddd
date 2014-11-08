#ifndef CDDD_CQRS_EVENT_STORE_H__
#define CDDD_CQRS_EVENT_STORE_H__

#include "cqrs/source.h"


namespace cddd {
namespace cqrs {

typedef source<std::shared_ptr<event>> event_source;

}
}

#endif
