#ifndef CDDD_CQRS_ARTIFACT_TRAITS_H__
#define CDDD_CQRS_ARTIFACT_TRAITS_H__

#include "cqrs/exceptions.h"

namespace cddd {
namespace cqrs {

template<class T, class F, typename=void>
class artifact_traits {
public:
   typedef T artifact_type;
   typedef F factory_type;
   typedef decltype(std::declval<F>()(std::declval<object_id>())) pointer;
   struct memento_type {};

   static inline object_id id_of(const T &object) {
      return object.id();
   }

   static inline std::size_t revision_of(const T &object) {
      return object.revision();
   }

   static inline event_sequence uncommitted_events_of(const T &object) {
      return object.uncommitted_events();
   }

   static inline void clear_uncommitted_events(T &object) {
      object.clear_uncommitted_events();
   }

   static inline void load_artifact_from_history(T &object, event_sequence events) {
      object.load_from_history(std::move(events));
   }

   static inline bool does_artifact_have_uncommitted_events(const T &object) {
      return object.has_uncommitted_events();
   }

   static inline void apply_memento_to_object(T &, std::size_t, store<memento_type> &) {
   }

   static inline void validate_artifact(pointer object) {
      if (object == nullptr) {
         throw null_pointer_exception("object");
      }
      else if (id_of(*object).is_null()) {
         throw null_id_exception("object->id()");
      }
   }

   static inline void validate_object_id(const object_id &id) {
      if (id.is_null()) {
         throw null_id_exception("id");
      }
   }
};


template<class T, class F>
class artifact_traits<T, F, typename T::memento_type> : artifact_traits<T, F, void> {
public:
   using artifact_traits<T, void>::artifact_type;
   using artifact_traits<T, void>::pointer;
   typedef typename T::memento_type memento_type;

   static inline void apply_memento_to_object(T &object, std::size_t revision, store<memento_type> &memento_provider) {
      if (memento_provider.has(id_of(object))) {
         object.apply_memento(memento_provider.get(id_of(object), revision));
      }
   }
};

}
}

#endif
