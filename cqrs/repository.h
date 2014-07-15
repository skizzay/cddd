#ifndef CDDD_CQRS_REPOSITORY_H__
#define CDDD_CQRS_REPOSITORY_H__

#include "cddd/cqrs/store.h"
#include "cddd/cqrs/stream.h"
#include "cddd/cqrs/traits.h"


namespace cddd {
namespace cqrs {

template<class T, class Stream=stream<T>, class Store=store<T>>
class repository : public Stream, public Store {
   static_assert(is_stream<Stream>::value,
                 "Stream must have event_sequence load() const and void save(event_sequence) member functions.");
   static_assert(is_store<Store>::value,
                 "Store must be store<T>.");

public:
   typedef std::shared_ptr<T> pointer;

   virtual ~repository() = default;
};

}
}

#endif
