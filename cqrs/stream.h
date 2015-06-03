#ifndef CDDD_CQRS_STREAM_H__
#define CDDD_CQRS_STREAM_H__

#include "cqrs/commit.h"
#include <sequence.h>

namespace cddd {
namespace cqrs {

template<class T>
class stream {
public:
   typedef T value_type;

   virtual ~stream() = default;

   sequencing::sequence<value_type> load() const {
      return load(1, std::numeric_limits<std::size_t>::max());
   }
   virtual sequencing::sequence<value_type> load(std::size_t min_version, std::size_t max_version) const = 0;
   virtual void save(sequencing::sequence<value_type> &objects) = 0;
   virtual commit<value_type> persist() = 0;
};

}
}

#endif
