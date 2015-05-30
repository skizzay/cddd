#include "cqrs/artifact.h"
#include "cqrs/fakes/fake_event.h"
#include "messaging/dispatcher.h"
#include <gmock/gmock.h>


// The purpose of these tests is to ensure that our basic_artifact and artifact
// provides the base functionality that end users expect.  Conversely, the
// artifact tests that the inherited classes execute ensure that the behavior is
// consistent among all artifacts.
namespace {

using namespace cddd::cqrs;


class artifact_spy : public artifact {
public:
   inline artifact_spy() :
      artifact{std::make_shared<cddd::messaging::dispatcher<>>()}
   {
   }

   template<class Fun>
   inline void handle(Fun &&f) {
      add_handler(std::forward<Fun>(f));
   }
};


class artifact_test : public ::testing::Test {
public:
   inline auto create_target() {
      return artifact_spy{};
   }
};


TEST_F(artifact_test, apply_change_does_invoke_dispatcher_to_dispatch_events) {
   // Given
   auto target = create_target();
   fake_event event;
   bool invoked = false;
   target.handle([&](const fake_event &) { invoked = true; });

   // When
   target.apply_change(std::move(event));

   // Then
   ASSERT_TRUE(invoked);
}


TEST_F(artifact_test, has_uncommitted_events_returns_false_when_collection_is_empty) {
   // Given
   auto target = create_target();

   // When
   bool actual = target.has_uncommitted_events();

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(artifact_test, has_uncommitted_events_returns_true_when_collection_is_not_empty) {
   // Given
   auto target = create_target();
   target.apply_change(fake_event{});

   // When
   bool actual = target.has_uncommitted_events();

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(artifact_test, revision_returns_0_without_applying_an_event) {
   // Given
   size_t expected = 0;
   auto target = create_target();

   // When
   auto actual = target.revision();

   // Then
   ASSERT_EQ(expected, actual);
}


TEST_F(artifact_test, uncommitted_events_returns_a_sequence_with_a_single_event) {
   // Given
   size_t expected = 1;
   auto target = create_target();
   fake_event e;
   target.apply_change(std::move(e));

   // When
   auto actual = target.uncommitted_events() | sequencing::count();

   // Then
   ASSERT_EQ(expected, actual);
}


TEST_F(artifact_test, has_uncommitted_events_returns_false_after_applying_an_event_and_clearing) {
   // Given
   auto target = create_target();
   fake_event e;
   target.apply_change(std::move(e));
   target.clear_uncommitted_events();

   // When
   bool actual = target.has_uncommitted_events();

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(artifact_test, uncommitted_events_returns_a_sequence_without_any_events_after_clearing) {
   // Given
   size_t expected = 0;
   auto target = create_target();
   fake_event e;
   target.apply_change(std::move(e));
   target.clear_uncommitted_events();

   // When
   auto actual = target.uncommitted_events() | sequencing::count();

   // Then
   ASSERT_EQ(expected, actual);
}

}
