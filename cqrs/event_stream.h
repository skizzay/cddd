#ifndef CDDD_CQRS_EVENT_STREAM_H__
#define CDDD_CQRS_EVENT_STREAM_H__

#include "cddd/cqrs/object_id.h"
#include "cddd/cqrs/event.h"


namespace cddd {
namespace cqrs {

class event_stream {
public:
   virtual ~event_stream() = default;

   virtual const object_id & id() const = 0;
   virtual std::size_t revision() const = 0;
   virtual std::size_t commit_sequence() const = 0;
   virtual void add(std::shared_ptr<event> evt) = 0;
   virtual void commit(object_id commit_id) = 0;
   virtual void clear_changes() = 0;
};

}
}

#endif
