#ifndef CDDD_UTILS_TABLE_POLICIES_H__
#define CDDD_UTILS_TABLE_POLICIES_H__

#include <map>
#include <unordered_map>

namespace cddd {
namespace utils {

struct use_map {
   template<class K, class V> using table_type = std::map<K, V>;
};


struct use_multimap {
   template<class K, class V> using table_type = std::multimap<K, V>;
};


struct use_unordered_map {
   template<class K, class V> using table_type = std::unordered_map<K, V>;
};


struct use_unordered_multimap {
   template<class K, class V> using table_type = std::unordered_multimap<K, V>;
};

}
}

#endif
