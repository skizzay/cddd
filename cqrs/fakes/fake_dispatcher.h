#ifndef CDDD_CQRS_FAKE_DISPATCHER_H__
#define CDDD_CQRS_FAKE_DISPATCHER_H__

#include "cqrs/event_dispatcher.h"
#include <gmock/gmock.h>

namespace cddd {
namespace cqrs {

class fake_dispatcher : public event_dispatcher {
public:
   virtual ~fake_dispatcher() = default;

   MOCK_METHOD2(add_handler, void(std::type_index, event_handler));
   MOCK_METHOD1(dispatch, void(event_ptr));
};

}
}

#endif
