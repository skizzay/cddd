#ifndef FAKE_EVENT_STREAM_H__
#define FAKE_EVENT_STREAM_H__

#include "cqrs/event_stream.h"
#include "cqrs/commit.h"
#include <deque>

namespace cddd {
namespace cqrs {

class event_stream_spy {
public:
   MOCK_CONST_METHOD0(load, void());
   MOCK_CONST_METHOD2(load, void(std::size_t, std::size_t));
   MOCK_METHOD0(save, void());
   MOCK_METHOD0(persist, void());
};


class fake_event_stream : public event_stream {
public:
   typedef std::shared_ptr<event> event_pointer;

   explicit inline fake_event_stream(std::shared_ptr<event_stream_spy> spy_=std::make_shared<event_stream_spy>()) :
      event_stream(),
      commitID(),
      sequenceID(),
      version(0),
      sequenceNumber(0),
      time_of_commit(),
      committed_events_script(),
      spy(spy_)
   {
   }
   virtual ~fake_event_stream() = default;

   virtual std::experimental::sequence<event_pointer> load(std::size_t min_version, std::size_t max_version) const final override {
      spy->load(min_version, max_version);
      return std::experimental::from(committed_events_script)
               >> std::experimental::where([=](event_pointer evt) { return min_version <= evt->version() && evt->version() <= max_version; });
   }

   virtual void save(std::experimental::sequence<value_type> events) final override {
      spy->save();
      std::copy(events.begin(), events.end(), std::back_inserter(committed_events_script));
   }
   
   virtual commit<event_pointer> persist() final override {
      spy->persist();
      return commit<value_type>{commitID, sequenceID, version, sequenceNumber, std::experimental::from(committed_events_script), time_of_commit};
   }

   object_id commitID;
   object_id sequenceID;
   std::size_t version;
   std::size_t sequenceNumber;
   commit<value_type>::time_point time_of_commit;
   std::deque<value_type> committed_events_script;
   std::shared_ptr<event_stream_spy> spy;
};


class event_stream_factory_spy {
public:
   MOCK_CONST_METHOD1(create_fake_stream, std::shared_ptr<fake_event_stream>(object_id));
};


class fake_event_stream_factory {
public:
   explicit inline fake_event_stream_factory(std::shared_ptr<event_stream_factory_spy> spy_=std::make_shared<event_stream_factory_spy>()) :
      spy(spy_)
   {
   }

   inline std::shared_ptr<fake_event_stream> operator()(object_id id) const {
      return spy->create_fake_stream(id);
   }

   std::shared_ptr<event_stream_factory_spy> spy;
};

}
}

#endif
