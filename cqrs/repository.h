#ifndef CDDD_CQRS_REPOSITORY_H__
#define CDDD_CQRS_REPOSITORY_H__

#include "cddd/cqrs/object_id.h"


namespace cddd {
namespace cqrs {

template<class T>
class repository<T> {
public:
   virtual ~repository() = default;

   virtual bool has(object_id) const = 0;
   virtual void save(T &) = 0;
   virtual std::shared_ptr<T> load(object_id) = 0;
};

}
}

#endif
