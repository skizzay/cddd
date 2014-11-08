#ifndef CDDD_CQRS_SOURCE_H__
#define CDDD_CQRS_SOURCE_H__

#include "cqrs/store.h"
#include "cqrs/stream.h"


namespace cddd {
namespace cqrs {

template<class T> using source = store<std::shared_ptr<stream<T>>>;

}
}

#endif
