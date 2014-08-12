#ifndef FAKE_EVENT_STREAM_H__
#define FAKE_EVENT_STREAM_H__

#include "cqrs/event_stream.h"
#include "cqrs/commit.h"
#include <deque>


class event_stream_spy {
public:
   MOCK_CONST_METHOD0(load, void());
   MOCK_CONST_METHOD2(load, void(std::size_t, std::size_t));
   MOCK_METHOD0(save, void());
   MOCK_METHOD0(persist, void());
};


class fake_event_stream : public cddd::cqrs::event_stream {
public:
   virtual ~fake_event_stream() = default;

   virtual std::experimental::sequence<pointer> load() const final override {
      spy->load();
      return std::experimental::from(committed_events_script);
   }

   virtual std::experimental::sequence<pointer> load(std::size_t min_version, std::size_t max_version) const final override {
      spy->load(min_version, max_version);
      return std::experimental::from(committed_events_script)
               >> std::experimental::where([=](pointer evt) { return min_version <= evt->version() && evt->version() <= max_version; });
   }

   virtual void save(std::experimental::sequence<pointer> events) final override {
      spy->save();
      std::copy(events.begin(), events.end(), std::back_inserter(committed_events_script));
   }
   
   virtual cddd::cqrs::commit<pointer> persist() final override {
      return cddd::cqrs::commit<pointer>{commitID, sequenceID, version, sequenceNumber, std::experimental::from(committed_events_script),
         time_of_commit};
   }

   cddd::cqrs::object_id commitID;
   cddd::cqrs::object_id sequenceID;
   std::size_t version = 0;
   std::size_t sequenceNumber = 0;
   cddd::cqrs::commit<pointer>::time_point time_of_commit;
   std::deque<pointer> committed_events_script;
   std::shared_ptr<::testing::NiceMock<event_stream_spy>> spy = std::make_shared<::testing::NiceMock<event_stream_spy>>();
};

#endif
