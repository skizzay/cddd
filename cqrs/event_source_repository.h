#ifndef CDDD_CQRS_EVENT_SOURCE_REPOSITORY_H__
#define CDDD_CQRS_EVENT_SOURCE_REPOSITORY_H__

#include "cddd/cqrs/repository.h"
#include "cddd/cqrs/event_store.h"


namespace cddd {
namespace cqrs {

template<class T>
class event_source_repository : public repository<T> {
public:
   typedef std::function<std::shared_ptr<T>(object_id)> object_factory;

   explicit inline event_source_repository(event_store_ptr es, object_id_generator id_gen, object_factory create_obj) :
      repository<T>(),
      store(es),
      generate_commit_id(std::move(id_gen)),
      create_object(std::move(create_obj))
   {
   }

   virtual ~event_source_repository() = default;

   virtual bool has(object_id id) const final override {
      return open_stream(id)->has_committed_events();
   }

   virtual std::shared_ptr<commit> save(T &object) final override {
      std::shared_ptr<commit> result;
      auto stream = open_stream(object.id());

      if (object.has_uncommitted_events()) {
         for (auto evt : object.uncommitted_events()) {
            stream->add_event(evt);
         }
         result = stream->commit_events(generate_commit_id());
         object.clear_uncommitted_events();

         return result;
      }

      return std::unique_ptr<commit>(nullptr);
   }

   virtual std::shared_ptr<T> load(object_id id) final override {
      auto object = create_object(id);
      object->load_from_history(*open_stream(id));
      return object;
   }

private:
   inline event_stream_ptr open_stream(object_id id) const {
      return store->open_stream(id);
   }

   event_store_ptr store;
   object_id_generator generate_commit_id;
   object_factory create_object;
};

}
}

#endif
