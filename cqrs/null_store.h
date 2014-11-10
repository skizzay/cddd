#ifndef CDDD_CQRS_NULL_STORE_H__
#define CDDD_CQRS_NULL_STORE_H__

#include "cqrs/store.h"
#include "cqrs/exceptions.h"

namespace cddd {
namespace cqrs {

template<class T>
class null_store : public store<T> {
public:
   virtual ~null_store() = default;

   virtual bool has(object_id) const {
      return false;
   }

   virtual T get(object_id id, std::size_t) const {
      throw aggregate_not_found(id);
   }

   virtual void put(T) {
   }
};

}
}

#endif
