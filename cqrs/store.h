#ifndef CDDD_CQRS_STORE_H__
#define CDDD_CQRS_STORE_H__

#include "cddd/cqrs/object_id.h"
#include "sequence.h"


namespace cddd {
namespace cqrs {

template<class T>
class store {
public:
   typedef T value_type;
   typedef std::shared_ptr<T> pointer;

   virtual ~store() = default;

   virtual bool has(object_id id) const = 0;
   virtual pointer get(object_id id) const = 0;
   virtual void put(pointer object) = 0;
};

}
}

#endif
