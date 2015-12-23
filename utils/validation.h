// vim: sw=3 ts=3 expandtab smartindent autoindent cindent
#pragma once

#include "utils/exceptions.h"

namespace cddd {
namespace utils {
namespace details_ {

template<class T>
struct is_uuid : std::is_same<boost::uuids::uuid, std::remove_cv_t<std::remove_reference_t<T>>> {};

}

inline bool is_valid(const boost::uuids::uuid &id) noexcept {
   return !(id.is_nil() || (id.version() == boost::uuids::uuid::version_unknown));
}


inline void do_validate_id_(const boost::uuids::uuid &id, std::experimental::string_view parameter) {
   if (id.is_nil()) {
      throw null_id_exception{parameter};
   }
   else if (id.version() == boost::uuids::uuid::version_unknown) {
      throw malformed_id_exception{id, parameter};
   }
}

#define validate_id(id) do_validate_id_(id, #id)

template<class T>
class not_null final {
   T pointer;

public:
   constexpr not_null(T pointer) :
      pointer{pointer}
   {
      assert((pointer != nullptr) && "Null pointer is not allowed.");
   }

   constexpr operator T () noexcept {
      return pointer;
   }
};


template<class T, class=void>
class valid_uuid final {
   static_assert(details_::is_uuid<T>::value, "Input type must be a uuid.");
};


template<class T>
class valid_uuid<T, std::enable_if_t<details_::is_uuid<T>::value>> final {
   const std::remove_reference_t<T> &object;

public:
   constexpr valid_uuid(const std::remove_reference_t<T> &object) :
      object{object}
   {
      assert(!object.is_nil() && "Cannot have nil uuid.");
      assert((object.version() != boost::uuids::uuid::version_unknown) && "Invalid uuid.");
   }

   constexpr operator const std::remove_reference_t<T> &() noexcept {
      return object;
   }
};

}
}
