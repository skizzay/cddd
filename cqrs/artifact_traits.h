#pragma once

#include "cqrs/store.h"
#include "utils/validation.h"
#include <type_traits>

namespace cddd {
namespace cqrs {

template<class T, class F, typename=void>
class artifact_traits {
public:
   typedef T artifact_type;
   typedef F factory_type;
   typedef decltype(std::declval<F>()(std::declval<const boost::uuids::uuid &>())) pointer;
   struct memento_type {};

   static inline void apply_memento_to_object(T &, size_t, store<memento_type> &) {
   }

   static inline void validate_artifact(pointer object) {
      if (object == nullptr) {
         throw utils::null_pointer_exception{"object"};
      }
      utils::validate_id(object->id());
   }

   static inline void validate_object_id(const boost::uuids::uuid &id) {
      utils::validate_id(id);
   }
};


template<class T, class F>
class artifact_traits<T, F, typename T::memento_type> : artifact_traits<T, F, void> {
public:
   using artifact_traits<T, void>::artifact_type;
   using artifact_traits<T, void>::pointer;
   typedef typename T::memento_type memento_type;

   static inline void apply_memento_to_object(T &object, size_t revision, store<memento_type> &memento_provider) {
      if (memento_provider.has(object.id())) {
         object.apply_memento(memento_provider.get(object.id(), revision));
      }
   }
};

}
}
