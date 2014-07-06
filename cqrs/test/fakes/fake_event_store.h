#ifndef FAKE_EVENT_STORE_H__
#define FAKE_EVENT_STORE_H__

#include "cddd/cqrs/event_store.h"
#include <gmock/gmock.h>


class fake_event_store : public cddd::cqrs::event_store {
public:
   virtual ~fake_event_store() = default;

   MOCK_CONST_METHOD1(has, bool(cddd::cqrs::object_id));
   MOCK_CONST_METHOD1(get, pointer(cddd::cqrs::object_id));
   MOCK_METHOD1(put, void(cddd::cqrs::pointer));
};

#endif
