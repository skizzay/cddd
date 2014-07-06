#ifndef CDDD_CQRS_SOURCE_H__
#define CDDD_CQRS_SOURCE_H__

#include "cddd/cqrs/store.h"
#include "cddd/cqrs/stream.h"


namespace cddd {
namespace cqrs {

template<class T, class Store=store<stream<T>>>
class source : public Store {
public:
   typedef std::shared_ptr<T> pointer;
   typedef typename Store::value_type stream_type;

   virtual ~source() = default;
};

}
}

#endif
