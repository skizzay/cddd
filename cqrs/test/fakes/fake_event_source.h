#ifndef FAKE_EVENT_SOURCE_H__
#define FAKE_EVENT_SOURCE_H__

#include "cqrs/event_source.h"
#include <gmock/gmock.h>


class event_source_spy {
public:
   virtual ~event_source_spy() = default;

   MOCK_CONST_METHOD1(has, bool(cddd::cqrs::object_id));
   MOCK_CONST_METHOD1(get, std::shared_ptr<cddd::cqrs::event_stream>(cddd::cqrs::object_id));
   MOCK_CONST_METHOD2(get, std::shared_ptr<cddd::cqrs::event_stream>(cddd::cqrs::object_id, std::size_t));
   MOCK_METHOD1(put, void(std::shared_ptr<cddd::cqrs::event_stream>));
};


class fake_event_source : public cddd::cqrs::event_source {
public:
   using cddd::cqrs::event_source::value_type;
   using cddd::cqrs::event_source::pointer;
   using cddd::cqrs::event_source::stream_type;
   typedef ::testing::NiceMock<event_source_spy> spy_type;

   virtual ~fake_event_source() = default;

   virtual bool has(cddd::cqrs::object_id id) const final override {
      return spy->has(id);
   }

   virtual pointer get(cddd::cqrs::object_id id) const final override {
      return spy->get(id);
   }

   virtual pointer get(cddd::cqrs::object_id id, std::size_t version) const final override {
      return spy->get(id, version);
   }

   virtual void put(pointer object) final override {
      spy->put(object);
   }

   std::shared_ptr<spy_type> spy = std::make_shared<spy_type>();
};

#endif
