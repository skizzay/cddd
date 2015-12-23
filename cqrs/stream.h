// vim: sw=3 ts=3 expandtab smartindent autoindent cindent
#ifndef CDDD_CQRS_STREAM_H__
#define CDDD_CQRS_STREAM_H__

#include "cqrs/commit.h"
#include <limits>
#include <memory>
#include <range/v3/all.hpp>

namespace cddd {
namespace cqrs {

template<class Implementation, class Deleter=std::default_delete<Implementation>>
class stream {
public:
   explicit inline stream(std::unique_ptr<Implementation, Deleter> impl) :
      impl{std::move(impl)}
   {
   }

   inline auto load() const {
      std::size_t min_revision = 1;
      std::size_t max_revision = std::numeric_limits<std::size_t>::max();
      return load(min_revision, max_revision);
   }

   inline auto load(std::size_t min_revision, std::size_t max_revision) const {
      typedef decltype(impl->load(min_revision, max_revision)) range_type;

      return ((min_revision == 0) || (max_revision < min_revision)) ? range_type{} :
         impl->load(min_revision, max_revision);
   }

   template<class DomainEventContainer>
   inline void save(const DomainEventContainer &container) {
      impl->save(ranges::view::all(container));
   }

   inline commit persist() {
      return impl->persist_changes();
   }

private:
   std::unique_ptr<Implementation, Deleter> impl;
};

}
}

#endif
