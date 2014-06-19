#ifndef FAKE_EVENT_STREAM_H__
#define FAKE_EVENT_STREAM_H__

#include "cddd/cqrs/event_stream.h"
#include <gmock/gmock.h>


class fake_event_stream : public event_stream {
public:
   virtual ~fake_event_stream() = default;

   MOCK_CONST_METHOD0(id, const cddd::cqrs::object_id &());
   MOCK_CONST_METHOD0(revision, std::size_t());
   MOCK_METHOD1(add_event, void(cddd::cqrs::event_ptr));
   MOCK_METHOD1(commit_events, std::unique_ptr<cddd::cqrs::commit>(cddd::cqrs::object_id));
   MOCK_METHOD0(clear_changes, void());
   MOCK_CONST_METHOD0(committed_events, cddd::cqrs::event_sequence());
};

#endif
