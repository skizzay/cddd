#ifndef CDDD_CQRS_STREAM_H__
#define CDDD_CQRS_STREAM_H__

#include "sequence.h"


namespace cddd {
namespace cqrs {

template<class T>
class stream {
public:
   typedef T value_type;
   typedef std::shared_ptr<T> pointer;

   virtual ~stream() = default;

   virtual std::experimental::sequence<pointer> load() const = 0;
   virtual void save(std::experimental::sequence<pointer> objects) = 0;
};

}
}

#endif
