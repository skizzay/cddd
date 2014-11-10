#ifndef CDDD_CQRS_POINTER_TRAITS_H__
#define CDDD_CQRS_POINTER_TRAITS_H__

#include <memory>

namespace cddd {
namespace cqrs {

template<class T>
struct shared_pointer_traits {
   typedef std::shared_ptr<T> pointer;

   template<class Alloc>
   static inline pointer make_pointer(const Alloc &alloc, object_id id) {
      return std::allocate_shared(alloc, id);
   }
};


template<class T, class D=std::default_delete<T>>
struct unique_pointer_traits {
   typedef std::unique_ptr<T, D> pointer;

   template<class Alloc>
   static inline pointer make_pointer(Alloc alloc, object_id id) {
      auto object = alloc.allocate(1);
      alloc.construct(object, std::move(id));
      return pointer{object};
   }
};

}
}

#endif
