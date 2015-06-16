#pragma once

#include <type_traits>

namespace cddd {
namespace cqrs {

template<class T, class K>
class table {
public:
   virtual ~table() = default;

   virtual bool has(const K &key) const = 0;
   virtual T get(const K &key) const = 0;
   virtual void put(std::decay_t<T> object) = 0;
};


template<class T>
using collection = table<T, const T>;


template<class, class>
struct is_collection : std::false_type {
};


template<class T>
struct is_collection<T, const T> : std::true_type {
};

}
}
