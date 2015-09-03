#pragma once

#include <boost/uuid/uuid.hpp>
#include <type_traits>

namespace cddd {
namespace cqrs {

template<class T, class K>
class table {
public:
   virtual ~table() = default;

   virtual bool has(const K &key) const = 0;
   virtual const T & get(const K &key) const = 0;
   virtual void put(const T &object, const K &key) = 0;
};


template<class T>
using collection = table<T, const boost::uuids::uuid>;


template<class, class>
struct is_collection : std::false_type {
};


template<class T>
struct is_collection<T, const T> : std::true_type {
};

}
}
