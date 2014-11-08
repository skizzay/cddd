#ifndef CDDD_CQRS_ARTIFACT_TRAITS_H__
#define CDDD_CQRS_ARTIFACT_TRAITS_H__

#include "cqrs/exceptions.h"

namespace cddd {
namespace cqrs {

template<class T>
struct artifact_traits {
   typedef T artifact_type;
   typedef std::shared_ptr<T> pointer;

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

}
}

#endif
