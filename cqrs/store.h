#ifndef CDDD_CQRS_STORE_H__
#define CDDD_CQRS_STORE_H__

#include "cqrs/table.h"
#include <boost/uuid/uuid.hpp>
#include <limits>

namespace cddd {
namespace cqrs {

template<class T, class K=const boost::uuids::uuid>
class store : public table<T, K> {
public:
   typedef T value_type;
   typedef K key_type;

   virtual ~store() = default;

   virtual T get(const key_type &id) const final override {
      return get(id, std::numeric_limits<std::size_t>::max());
   }
   virtual T get(const key_type &id, std::size_t version) const = 0;
};

}
}

#endif
