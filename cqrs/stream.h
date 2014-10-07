#ifndef CDDD_CQRS_STREAM_H__
#define CDDD_CQRS_STREAM_H__

#include "sequence.h"
#include "cqrs/commit.h"


namespace cddd {
namespace cqrs {

template<class T, class Ptr=std::shared_ptr<T>>
class stream {
public:
   typedef T value_type;
   typedef Ptr pointer;

   virtual ~stream() = default;

   virtual std::experimental::sequence<pointer> load() const = 0;
   virtual void save(std::experimental::sequence<pointer> objects) = 0;
   virtual commit<pointer> persist() = 0;
};

}
}

#endif
