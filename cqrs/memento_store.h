#pragma once

#include <cstdint>

namespace cddd {
namespace cqrs {
namespace details_ {

template<class ... Args>
using void_t = void;
}

template<class T, class=void>
class memento_store final {
public:
   static inline void apply_memento_to_object(T &, std::size_t, memento_store &) {
   }

   static inline memento_store & instance() {
      static memento_store singleton;
      return singleton;
   }
};


template<class T>
class memento_store<T, details_::void_t<typename T::memento_type>> {
public:
   virtual ~memento_store() noexcept = default;

   static inline void apply_memento_to_object(T &object, std::size_t revision, memento_store &store) {
      store.apply_memento_to_object(object, revision);
   }

   virtual void apply_memento_to_object(T &object, std::size_t revision) = 0;
};

}
}
