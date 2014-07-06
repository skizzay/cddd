#ifndef CDDD_CQRS_REPOSITORY_H__
#define CDDD_CQRS_REPOSITORY_H__

#include "cddd/cqrs/store.h"
#include "cddd/cqrs/stream.h"


namespace cddd {
namespace cqrs {

template<class T, class Stream=stream<T>, class Store=store<T>>
class repository : public Stream, public Store {
   static_assert(std::is_base_of<stream<T>, Stream>::value || std::is_same<stream<T>, Stream>::value,
                 "Stream must inherit/be stream<T>.");
   static_assert(std::is_base_of<store<T>, Store>::value || std::is_same<store<T>, Store>::value,
                 "Store must inherit/be store<T>.");

public:
   typedef std::shared_ptr<T> pointer;

   virtual ~repository() = default;
};

}
}

#endif
