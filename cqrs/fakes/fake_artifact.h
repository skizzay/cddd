#ifndef CDDD_CQRS_FAKE_ARTIFACT_H__
#define CDDD_CQRS_FAKE_ARTIFACT_H__

#include "cqrs/artifact.h"
#include "cqrs/test/fakes/fake_event.h"
#include <gmock/gmock.h>

namespace cddd {
namespace cqrs {

class fake_artifact : public artifact {
public:
   struct memento_type{};

   explicit inline fake_artifact(std::shared_ptr<event_dispatcher> d) :
      artifact(d)
   {
      add_handler<fake_event>(std::bind(&fake_artifact::on_fake_event, this,
                              std::placeholders::_1));
   }
   virtual ~fake_artifact() = default;

   MOCK_CONST_METHOD0(uncommitted_events, event_sequence());
   MOCK_CONST_METHOD0(has_uncommitted_events, bool());
   MOCK_CONST_METHOD0(size_uncommitted_events, size_type());
   MOCK_METHOD0(clear_uncommitted_events, void());
   MOCK_METHOD1(add_pending_event, void(event_ptr));

   // For faking event callbacks.
   MOCK_METHOD1(on_fake_event, void(fake_event));

   MOCK_METHOD(apply_memento, void(memento_type));
};

}
}

#endif
