#ifndef CDDD_CQRS_EVENT_SOURCE_REPOSITORY_H__
#define CDDD_CQRS_EVENT_SOURCE_REPOSITORY_H__

#include "cddd/cqrs/repository.h"
#include "cddd/cqrs/event_store.h"


namespace cddd {
namespace cqrs {

template<class T>
class event_source_repository : public repository<T> {
public:
   static_assert(std::is_base_of<artifact, T>::value,
                 "T must inherit from artifact.");
   typedef std::function<artifact_ptr(object_id)> artifact_factory;

   explicit inline event_source_repository(event_store_ptr es, object_id_generator id_gen, artifact_factory create_obj) :
      repository<T>(),
      store(es),
      generate_commit_id(std::move(id_gen)),
      create_object(std::move(create_obj))
   {
   }

   virtual ~event_source_repository() = default;

   virtual bool has(object_id id) const final override {
      return !open_stream(id)->committed_events().empty();
   }

   virtual std::unique_ptr<commit> save(T &object) final override {
      auto stream = open_stream(object.id());
      for (auto evt : object.uncommitted_events()) {
         stream->add_event(evt);
      }
      auto result = stream->commit_events(generate_commit_id());
      object.clear_uncommitted_events();

      return result;
   }

   virtual std::shared_ptr<T> load(object_id id) final override {
      artifact_ptr object = create_object(id);
      auto stream = open_stream(id);
      object->load_from_history(*stream);
      return std::static_pointer_cast<T>(object);
   }

private:
   inline event_stream_ptr open_stream(object_id id) {
      return store->open_stream(id);
   }

   event_store_ptr store;
   object_id_generator generate_commit_id;
   artifact_factory create_object;
};

}
}

#endif
