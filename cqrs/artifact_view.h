#pragma once

#include "cqrs/artifact.h"


namespace cddd {
namespace cqrs {

template<class> class basic_artifact_view;

template<class DomainEventDispatcher, class DomainEventContainer>
class basic_artifact_view<basic_artifact<DomainEventDispatcher, DomainEventContainer>> {
public:
   using id_type = typename basic_artifact<DomainEventDispatcher, DomainEventContainer>::id_type;
   using size_type = typename basic_artifact<DomainEventDispatcher, DomainEventContainer>::size_type;

   const id_type &id() const {
      return artifact_.id();
   }

   size_type revision() const {
      return artifact_.revision();
   }

   template<class Evt>
   inline auto apply_change(Evt &&e) {
      using std::forward;
      return artifact_.apply_change(forward<Evt>(e));
   }

protected:
   explicit inline basic_artifact_view(basic_artifact<DomainEventDispatcher, DomainEventContainer> &a) :
      artifact_{a}
   {
   }

   template<class Fun>
   void add_handler(Fun f) {
      using std::move;
      artifact_.add_handler(move(f));
   }

private:
   basic_artifact<DomainEventDispatcher, DomainEventContainer> &artifact_;
};


typedef basic_artifact_view<artifact> artifact_view;

}
}
