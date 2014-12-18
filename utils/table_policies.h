#ifndef CDDD_UTILS_TABLE_POLICIES_H__
#define CDDD_UTILS_TABLE_POLICIES_H__

#include <map>
#include <unordered_map>

namespace cddd {
namespace utils {

struct use_map {
   template<class K, class V> using table_type = std::map<K, V>;

   template<class Iterator>
   static inline bool is_insertion_successful(const std::pair<Iterator, bool> &result) {
      return result.second;
   }
};


struct use_multimap {
   template<class K, class V> using table_type = std::multimap<K, V>;

   template<class Iterator>
   static inline bool is_insertion_successful(const Iterator &) {
      return true;
   }
};


struct use_unordered_map {
   template<class K, class V> using table_type = std::unordered_map<K, V>;

   template<class Iterator>
   static inline bool is_insertion_successful(const std::pair<Iterator, bool> &result) {
      return result.second;
   }
};


struct use_unordered_multimap {
   template<class K, class V> using table_type = std::unordered_multimap<K, V>;

   template<class Iterator>
   static inline bool is_insertion_successful(const Iterator &) {
      return true;
   }
};

}
}

#endif
