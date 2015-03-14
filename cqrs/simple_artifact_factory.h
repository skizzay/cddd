#ifndef CDDD_CQRS_SIMPLE_ARTIFACT_FACTORY_H__
#define CDDD_CQRS_SIMPLE_ARTIFACT_FACTORY_H__

#include "cqrs/traits.h"

namespace cddd {
namespace cqrs {

template<class T, class Alloc=std::allocator<T>, class PointerTraits=shared_pointer_traits<T>>
class simple_artifact_factory {
public:
   typedef typename Alloc::template rebind<T>::other allocator_type;
   typedef typename PointerTraits::pointer pointer;

   explicit inline simple_artifact_factory(allocator_type alloc_=allocator_type{}) :
      alloc(alloc_)
   {
   }

   pointer operator()(const boost::uuids::uuid &id) const {
      return PointerTraits::make_pointer(alloc, std::move(id));
   }

private:
   allocator_type alloc;
};

}
}

#endif
