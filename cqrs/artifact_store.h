#ifndef CDDD_CQRS_ARTIFACT_STORE_H__
#define CDDD_CQRS_ARTIFACT_STORE_H__

#include "cqrs/event.h"
#include "cqrs/source.h"
#include "cqrs/traits.h"

namespace cddd {
namespace cqrs {

template<class ArtifactType, class StreamFactory, class ArtifactFactory, class Traits=artifact_traits<ArtifactType>>
class artifact_store : public store<std::shared_ptr<ArtifactType>> {
public:
   typedef Traits traits_type;
   typedef typename traits_type::artifact_type value_type;
   typedef typename traits_type::pointer pointer;
   typedef source<std::shared_ptr<event>> event_source;
   typedef stream<std::shared_ptr<event>> event_stream;
   typedef StreamFactory stream_factory;
   typedef ArtifactFactory artifact_factory;
   using store<std::shared_ptr<ArtifactType>>::get;

   inline artifact_store(std::shared_ptr<event_source> es, stream_factory sf, artifact_factory of) :
      events_provider(es),
      create_stream(std::move(sf)),
      create_artifact(std::move(of)) {
   }

   artifact_store(const artifact_store &) = delete;
   artifact_store(artifact_store &&) = default;

   virtual ~artifact_store() = default;

   artifact_store &operator =(const artifact_store &) = delete;
   artifact_store &operator =(artifact_store &&) = default;

   virtual bool has(object_id id) const final override {
      return !id.is_null() && events_provider->has(id);
   }

   virtual void put(pointer object) final override {
      traits_type::validate_artifact(object);

      if (traits_type::does_artifact_have_uncommitted_events(*object)) {
         save_object(*object);
      }
   }

   virtual pointer get(object_id id, std::size_t version) const override {
      traits_type::validate_object_id(id);
      return load_object(id, version);
   }

private:
   inline void save_object(ArtifactType &object) {
      auto str = get_event_stream(object.id());
      str->save(traits_type::uncommitted_events_of(object));
      events_provider->put(str);
      traits_type::clear_uncommitted_events(object);
   }

   inline std::shared_ptr<event_stream> get_event_stream(object_id id) {
      return has(id) ? events_provider->get(id) : create_stream(id);
   }

   inline event_sequence load_events(object_id id, std::size_t min_revision, std::size_t max_revision) const {
      return events_provider->get(id)->load(min_revision, max_revision);
   }

   inline auto load_object(object_id id, std::size_t revision) const {
      auto object = create_artifact(id, revision);
      if (traits_type::revision_of(*object) < revision) {
         traits_type::load_artifact_from_history(*object, load_events(id, traits_type::revision_of(*object) + 1, revision));
      }
      return object;
   }

   std::shared_ptr<event_source> events_provider;
   stream_factory create_stream;
   artifact_factory create_artifact;
};

}
}

#endif
