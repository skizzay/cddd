#pragma once

#include "cqrs/domain_event.h"
#include "cqrs/stream.h"
#include <boost/uuid/uuid.hpp>
#include <cassert>
#include <type_traits>

namespace cddd {
namespace cqrs {

template<class Derived>
class domain_event_stream_store {
public:
   inline bool has(const boost::uuids::uuid &id) const {
      return !id.is_nil() && static_cast<const Derived *>(this)->has_stream_for(id);
   }

   inline auto get_or_create(const boost::uuids::uuid &id) {
      return id.is_nil() ? nullptr :
         static_cast<const Derived *>(this)->has_stream_for(id) ?
            static_cast<Derived *>(this)->get_stream_for(id) :
            static_cast<Derived *>(this)->create_stream_for(id);
   }
};

}
}
