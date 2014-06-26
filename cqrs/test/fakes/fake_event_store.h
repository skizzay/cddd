#ifndef FAKE_EVENT_STORE_H__
#define FAKE_EVENT_STORE_H__

#include "cddd/cqrs/event_store.h"
#include <gmock/gmock.h>


class fake_event_store : public cddd::cqrs::event_store {
public:
   virtual ~fake_event_store() = default;

   MOCK_METHOD1(open_stream, cddd::cqrs::event_stream_ptr(cddd::cqrs::object_id));
};

#endif
