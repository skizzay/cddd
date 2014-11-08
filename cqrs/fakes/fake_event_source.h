#ifndef FAKE_EVENT_SOURCE_H__
#define FAKE_EVENT_SOURCE_H__

#include "cqrs/event_source.h"
#include "cqrs/event_stream.h"
#include <gmock/gmock.h>

namespace cddd {
namespace cqrs {

class event_source_spy {
public:
   virtual ~event_source_spy() = default;

   MOCK_CONST_METHOD1(has, bool(object_id));
   MOCK_CONST_METHOD2(get, std::shared_ptr<event_stream>(object_id, std::size_t));
   MOCK_METHOD1(put, void(std::shared_ptr<event_stream>));
};


class fake_event_source : public event_source {
public:
   using event_source::value_type;
   typedef ::testing::NiceMock<event_source_spy> spy_type;

   explicit inline fake_event_source(std::shared_ptr<spy_type> spy_=std::make_shared<spy_type>()) :
      event_source(),
      spy(spy_)
   {
   }
   virtual ~fake_event_source() = default;

   virtual bool has(object_id id) const final override {
      return spy->has(id);
   }

   virtual std::shared_ptr<event_stream> get(object_id id, std::size_t version) const final override {
      return spy->get(id, version);
   }

   virtual void put(std::shared_ptr<event_stream> stream) final override {
      spy->put(stream);
   }

   std::shared_ptr<spy_type> spy;
};

}
}

#endif
