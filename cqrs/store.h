#ifndef CDDD_CQRS_STORE_H__
#define CDDD_CQRS_STORE_H__

#include "sequence.h"
#include <boost/uuid/uuid.hpp>

namespace cddd {
namespace cqrs {

template<class T>
class store {
public:
   typedef T value_type;

   virtual ~store() = default;

   virtual bool has(const boost::uuids::uuid &id) const = 0;
   T get(const boost::uuids::uuid &id) const {
      return get(id, std::numeric_limits<std::size_t>::max());
   }
   virtual T get(const boost::uuids::uuid &id, std::size_t version) const = 0;
   virtual void put(std::decay_t<T> object) = 0;
};

}
}

#endif
