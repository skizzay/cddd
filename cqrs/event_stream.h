#ifndef CDDD_CQRS_EVENT_STREAM_H__
#define CDDD_CQRS_EVENT_STREAM_H__

#include "cddd/cqrs/commit.h"


namespace cddd {
namespace cqrs {

class event_stream {
public:
   virtual ~event_stream() = default;

   virtual const object_id & id() const = 0;
   virtual std::size_t revision() const = 0;
   virtual void add_event(event_ptr evt) = 0;
   virtual std::shared_ptr<commit> commit_events(object_id commit_id) = 0;
   virtual void clear_changes() = 0;

   virtual event_sequence committed_events() const = 0;
   virtual bool has_committed_events() const = 0;
};


typedef std::shared_ptr<event_stream> event_stream_ptr;

}
}

#endif
