// vim: sw=3 ts=3 expandtab smartindent autoindent cindent
#pragma once

#include "cqrs/domain_event.h"
#include "cqrs/stream.h"
#include "utils/validation.h"
#include <boost/uuid/uuid.hpp>
#include <cassert>
#include <type_traits>

namespace cddd {
namespace cqrs {
namespace details_ {

template<class T>
struct is_stream : std::false_type {};

template<class T, class D>
struct is_stream<stream<T, D>> : std::true_type {};

template<class T>
struct is_like_a_stream_pointer : std::integral_constant<bool, std::is_pointer<T>::value && is_stream<std::remove_pointer_t<T>>::value> {};

template<class T>
struct is_like_a_stream_pointer<std::shared_ptr<T>> : std::true_type {};

template<class T, class D>
struct is_like_a_stream_pointer<std::unique_ptr<T, D>> : std::true_type {};

}

template<class Implementation, class Deleter=std::default_delete<Implementation>>
class domain_event_stream_store {
   using stream_type = decltype(std::declval<Implementation>().get_stream_for(std::declval<boost::uuids::uuid>()));

public:
   static_assert(std::is_same<stream_type,
                              decltype(std::declval<Implementation>().create_stream_for(std::declval<boost::uuids::uuid>()))>::value,
                 "Implementation must provide the same result for get_stream_for and create_stream_for.");
   static_assert(details_::is_like_a_stream_pointer<stream_type>::value,
                 "Implementation must provide a stream that is a (smart) pointer to a stream of domain events.");

   explicit inline domain_event_stream_store(std::unique_ptr<Implementation, Deleter> impl) :
      impl{std::move(impl)}
   {
   }

   inline bool has(utils::valid_uuid<const boost::uuids::uuid &> id) const {
      return impl->has_stream_for(id);
   }

   inline utils::not_null<stream_type> get_or_create(utils::valid_uuid<const boost::uuids::uuid &> id) {
      return impl->has_stream_for(id) ?  impl->get_stream_for(id) : impl->create_stream_for(id);
   }

private:
   std::unique_ptr<Implementation, Deleter> impl;
};

}
}
