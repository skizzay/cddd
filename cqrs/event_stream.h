#ifndef CDDD_CQRS_EVENT_STREAM_H__
#define CDDD_CQRS_EVENT_STREAM_H__

#include "cqrs/stream.h"
#include "cqrs/event.h"
#include <limits>


namespace cddd {
namespace cqrs {

class basic_event_stream : public stream<event> {
public:
   virtual ~basic_event_stream() = default;

   virtual event_sequence load(std::size_t min_version, std::size_t max_version=std::numeric_limits<std::size_t>::max()) const = 0;
};


class event_stream : public basic_event_stream {
public:
   using basic_event_stream::load;

   virtual ~event_stream() = default;

   virtual event_sequence load() const override {
      return load(1, std::numeric_limits<std::size_t>::max());
   }
};


typedef std::shared_ptr<basic_event_stream> event_stream_ptr;

}
}

#endif
