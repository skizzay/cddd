#pragma once

#include "utils/exceptions.h"

namespace cddd {
namespace utils {

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

}
}
