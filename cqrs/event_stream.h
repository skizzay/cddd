#ifndef CDDD_CQRS_EVENT_STREAM_H__
#define CDDD_CQRS_EVENT_STREAM_H__

#include "cddd/cqrs/event.h"
#include "cddd/cqrs/stream.h"
#include <limits>


namespace cddd {
namespace cqrs {

#if 0
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
#endif

class basic_event_stream : public stream<event> {
public:
   virtual ~basic_event_stream() = default;

   virtual event_sequence load(std::size_t min_version, std::size_t max_version=std::numeric_limits<std::size_t>::max()) const = 0;
};


class event_stream : public basic_event_stream {
public:
   virtual ~event_stream() = default;

   virtual event_sequence load() const override {
      return load(0, std::numeric_limits<std::size_t>::max());
   }

   virtual event_sequence load(std::size_t min_version, std::size_t max_version=std::numeric_limits<std::size_t>::max()) const override {
      return this->load() >> where([min_version, max_version](pointer evt) {
            return min_version <= evt->version() && evt->version() <= max_version;
         });
   }
};


typedef std::shared_ptr<basic_event_stream> event_stream_ptr;

}
}

#endif
