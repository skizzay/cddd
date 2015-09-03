#pragma once

#include "cqrs/artifact.h"
#include "cqrs/commit.h"
#include "cqrs/domain_event.h"
#include "cqrs/domain_event_stream_store.h"
#include "cqrs/memento_store.h"
#include "cqrs/simple_artifact_factory.h"
#include "cqrs/traits.h"
#include "utils/validation.h"

namespace cddd {
namespace cqrs {

template<class ArtifactType, class DomainEventSource, class ArtifactFactory=simple_artifact_factory<ArtifactType>>
class artifact_store {
public:
   typedef boost::uuids::uuid key_type;
   typedef ArtifactType artifact_type;
   typedef ArtifactFactory artifact_factory;
   typedef DomainEventSource domain_event_source;
   typedef decltype(std::declval<artifact_factory>()(std::declval<const key_type &>())) pointer;

   inline artifact_store(domain_event_source &es, memento_store<ArtifactType> &ms, artifact_factory af) noexcept :
      event_stream_provider(es),
      memento_provider(ms),
      create_artifact(std::move(af)) {
   }

   inline artifact_store(domain_event_source &es, artifact_factory af) noexcept :
      event_stream_provider(es),
      memento_provider(memento_store<ArtifactType>::instance()),
      create_artifact(std::move(af)) {
   }

   artifact_store(const artifact_store &) = delete;
   artifact_store(artifact_store &&) = default;

   inline ~artifact_store() noexcept = default;

   artifact_store &operator =(const artifact_store &) = delete;
   artifact_store &operator =(artifact_store &&) = default;

   inline bool has(const key_type &id) const noexcept {
      return event_stream_provider.has(id);
   }

   inline commit put(ArtifactType &object) {
      utils::validate_id(object.id());
      if (object.has_uncommitted_events()) {
         commit result = save_object(object);
         object.clear_uncommitted_events();
         return result;
      }
      return commit::noncommit();
   }

   // When version==0, then we are essentially just creating an object.
   inline pointer get(const key_type &id, std::size_t version) {
      utils::validate_id(id);
      return load_object(id, version);
   }

private:
   inline commit save_object(const ArtifactType &object) {
      auto str = event_stream_provider.get_or_create(object.id());
      str->save(object.uncommitted_events());
      return str->persist();
   }

   inline auto load_events(const key_type &id, std::size_t min_revision, std::size_t max_revision) {
      return event_stream_provider.get_or_create(id)->load(min_revision, max_revision);
   }

   inline pointer load_object(const key_type &id, std::size_t revision) {
      pointer object = create_artifact(id);
      memento_store<ArtifactType>::apply_memento_to_object(*object, revision, memento_provider);
      std::size_t object_revision = object->revision();
      if (object_revision < revision) {
         object->load_from_history(load_events(id, object_revision + 1, revision));
      }
      return object;
   }

   domain_event_source &event_stream_provider;
   memento_store<ArtifactType> &memento_provider;
   artifact_factory create_artifact;
};

}
}
