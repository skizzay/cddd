#ifndef CDDD_CQRS_FAKE_DISPATCHER_H__
#define CDDD_CQRS_FAKE_DISPATCHER_H__

#include "cddd/cqrs/event_dispatcher.h"
#include <gmock/gmock.h>


class fake_dispatcher : public cddd::cqrs::event_dispatcher {
public:
   virtual ~fake_dispatcher() = default;

   MOCK_METHOD2(add_handler, void(std::type_index, cddd::cqrs::event_handler));
   MOCK_METHOD1(dispatch, void(cddd::cqrs::event_ptr));
};

#endif
