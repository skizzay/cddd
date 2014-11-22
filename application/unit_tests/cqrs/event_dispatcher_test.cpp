#include "cqrs/event_dispatcher.h"
#include "cqrs/fakes/fake_event.h"
#include <gtest/gtest.h>


namespace {

using namespace cddd::cqrs;


class event_dispatcher_test : public ::testing::Test {
public:
   typedef domain_event_dispatcher Target;

   inline Target create_target() const {
      return Target{};
   }
};


TEST_F(event_dispatcher_test, has_handler_returns_false_when_default_constructed) {
   // Given
   Target target{create_target()};

   // When
   bool actual = target.has_handler(typeid(fake_event));

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(event_dispatcher_test, has_handler_templated_returns_false_when_default_constructed) {
   // Given
   Target target{create_target()};

   // When
   bool actual = target.has_handler<fake_event>();

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(event_dispatcher_test, has_handler_returns_true_after_handler_has_been_added) {
   // Given
   Target target{create_target()};
   target.add_handler(typeid(fake_event), [](const domain_event &){});

   // When
   bool actual = target.has_handler(typeid(fake_event));

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(event_dispatcher_test, has_handler_templated_returns_after_handler_has_been_added) {
   // Given
   Target target{create_target()};
   target.add_handler(typeid(fake_event), [](const domain_event &){});

   // When
   bool actual = target.has_handler<fake_event>();

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(event_dispatcher_test, has_handler_returns_true_after_handler_has_been_added_templated) {
   // Given
   Target target{create_target()};
   target.add_handler<fake_event>([](fake_event){});

   // When
   bool actual = target.has_handler(typeid(fake_event));

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(event_dispatcher_test, has_handler_templated_returns_after_handler_has_been_added_templated) {
   // Given
   Target target{create_target()};
   target.add_handler<fake_event>([](fake_event){});

   // When
   bool actual = target.has_handler<fake_event>();

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(event_dispatcher_test, dispatch_fires_event_handler) {
   // Given
   Target target{create_target()};
   bool handler_fired = false;
   domain_event_ptr evt = std::make_shared<details_::domain_event_wrapper<fake_event>>(fake_event(), 1);
   target.add_handler<fake_event>([&handler_fired](const fake_event &){ handler_fired = true; });

   // When
   target.dispatch(*evt);

   // Then
   ASSERT_TRUE(handler_fired);
}

}
