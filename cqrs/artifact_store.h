#ifndef CDDD_CQRS_ARTIFACT_STORE_H__
#define CDDD_CQRS_ARTIFACT_STORE_H__

#include "cqrs/artifact.h"
#include "cqrs/domain_event.h"
#include "cqrs/null_store.h"
#include "cqrs/simple_artifact_factory.h"
#include "cqrs/source.h"
#include "cqrs/traits.h"
#include <iostream>

namespace cddd {
namespace cqrs {

template<class ArtifactType, class StreamFactory, class ArtifactFactory=simple_artifact_factory<ArtifactType>, class Traits=artifact_traits<ArtifactType, ArtifactFactory>>
class artifact_store : public store<typename Traits::pointer> {
public:
   static_assert(std::is_same<ArtifactType, base_artifact>::value || std::is_base_of<base_artifact, ArtifactType>::value,
                 "artifact_store can only store artifacts.");

   typedef Traits traits_type;
   typedef boost::uuids::uuid key_type;
   typedef typename traits_type::artifact_type value_type;
   typedef typename traits_type::pointer pointer;
   typedef typename traits_type::memento_type memento_type;
   typedef source<std::shared_ptr<domain_event>> domain_event_source;
   typedef stream<std::shared_ptr<domain_event>> domain_event_stream;
   typedef store<memento_type> memento_store;
   typedef StreamFactory stream_factory;
   typedef ArtifactFactory artifact_factory;
   using store<std::shared_ptr<ArtifactType>>::get;

   inline artifact_store(std::shared_ptr<domain_event_source> es, stream_factory sf, artifact_factory af, std::shared_ptr<memento_store> ms=std::make_shared<null_store<memento_type>>()) :
      events_provider(es),
      memento_provider(ms),
      create_stream(std::move(sf)),
      create_artifact(std::move(af)) {
   }

   artifact_store(const artifact_store &) = delete;
   artifact_store(artifact_store &&) = default;

   virtual ~artifact_store() = default;

   artifact_store &operator =(const artifact_store &) = delete;
   artifact_store &operator =(artifact_store &&) = default;

   virtual bool has(const key_type &id) const final override {
      return !id.is_nil() && events_provider->has(id);
   }

   virtual void put(pointer object) final override {
      traits_type::validate_artifact(object);

      if (traits_type::does_artifact_have_uncommitted_events(*object)) {
         save_object(*object);
      }
   }

   virtual pointer get(const key_type &id, std::size_t version) const override {
      traits_type::validate_object_id(id);
      return load_object(id, version);
   }

private:
   inline void save_object(ArtifactType &object) {
      auto str = get_event_stream(traits_type::id_of(object));
      auto events = traits_type::uncommitted_events_of(object);
      str->save(events);
      events_provider->put(std::move(str));
      traits_type::clear_uncommitted_events(object);
   }

   inline std::shared_ptr<domain_event_stream> get_event_stream(const key_type &id) {
      return has(id) ? events_provider->get(id) : create_stream(id);
   }

   inline domain_event_sequence load_events(const key_type &id, std::size_t min_revision, std::size_t max_revision) const {
      return events_provider->get(id)->load(min_revision, max_revision);
   }

   inline auto load_object(const key_type &id, std::size_t revision) const {
      pointer object = create_artifact(id);
      traits_type::apply_memento_to_object(*object, revision, *memento_provider);
      std::size_t object_revision = traits_type::revision_of(*object);
      if (object_revision < revision) {
         domain_event_sequence events = load_events(id, object_revision + 1, revision);
         traits_type::load_artifact_from_history(*object, events);
      }
      return object;
   }

   std::shared_ptr<domain_event_source> events_provider;
   std::shared_ptr<memento_store> memento_provider;
   stream_factory create_stream;
   artifact_factory create_artifact;
};

}
}

#endif
