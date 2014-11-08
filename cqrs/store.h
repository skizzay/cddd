#ifndef CDDD_CQRS_STORE_H__
#define CDDD_CQRS_STORE_H__

#include "cqrs/object_id.h"
#include "sequence.h"

namespace cddd {
namespace cqrs {

template<class T>
class store {
public:
   typedef T value_type;

   virtual ~store() = default;

   virtual bool has(object_id id) const = 0;
   T get(object_id id) const {
      return get(id, std::numeric_limits<std::size_t>::max());
   }
   virtual T get(object_id id, std::size_t version) const = 0;
   virtual void put(std::decay_t<T> object) = 0;
};

}
}

#endif
