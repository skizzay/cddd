#ifndef CDDD_CQRS_STREAM_H__
#define CDDD_CQRS_STREAM_H__

#include "cqrs/commit.h"
#include <sequence.h>

namespace cddd {
namespace cqrs {

template<class Derived>
class stream {
public:
   inline auto load() const {
      std::size_t min_revision = 1;
      std::size_t max_revision = std::numeric_limits<std::size_t>::max();
      return load(min_revision, max_revision);
   }

   inline auto load(std::size_t min_revision, std::size_t max_revision) const {
      typedef decltype(std::declval<const Derived &>().load_revisions(min_revision, max_revision)) sequence_type;

      return ((min_revision == 0) || (max_revision < min_revision)) ? sequence_type{} :
         static_cast<const Derived *>(this)->load_revisions(min_revision, max_revision);
   }

   template<class DomainEventContainer>
   inline void save(const DomainEventContainer &container) {
      using std::begin;
      using std::end;

      static_cast<Derived *>(this)->save_sequence(sequencing::from(begin(container), end(container)));
   }

   inline commit persist() {
      return static_cast<Derived *>(this)->persist_changes();
   }
};

}
}

#endif
