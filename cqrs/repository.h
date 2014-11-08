#ifndef CDDD_CQRS_REPOSITORY_H__
#define CDDD_CQRS_REPOSITORY_H__

#include "cqrs/store.h"
#include "cqrs/stream.h"

namespace cddd {
namespace cqrs {

template<class T>
class repository : public stream<T>, public store<T> {
public:
   typedef T value_type;

   virtual ~repository() = default;
};

}
}

#endif
