#include "cqrs/artifact_view.h"
#include "cqrs/fakes/fake_event.h"
#include <kerchow/kerchow.h>
#include <boost/uuid/uuid_generators.hpp>
#include <gtest/gtest.h>


namespace {

using namespace cddd::cqrs;

boost::uuids::basic_random_generator<decltype(kerchow::picker)> gen_id{kerchow::picker};


class artifact_spy : public artifact {
public:
   using artifact::basic_artifact;

   inline artifact_spy(const id_type &id_) :
      artifact{id_}
   {
   }
};


class artifact_view_spy : public artifact_view {
public:
   using artifact_view::basic_artifact_view;

   inline artifact_view_spy(artifact &a) :
      artifact_view{a}
   {
   }

   template<class Fun>
   inline void handle(Fun &&f) {
      add_handler(std::forward<Fun>(f));
   }
};


class artifact_view_test : public ::testing::Test {
public:
   inline auto create_target() {
      return artifact_view_spy{spy};
   }

   artifact_spy spy{gen_id()};
};


TEST_F(artifact_view_test, apply_change_on_artifact_does_invoke_dispatcher_to_dispatch_events) {
   // Given
   auto target = create_target();
   fake_event event;
   bool invoked = false;
   target.handle([&](const fake_event &) { invoked = true; });

   // When
   spy.apply_change(std::move(event));

   // Then
   ASSERT_TRUE(invoked);
}


TEST_F(artifact_view_test, revision_on_view_returns_0_without_applying_an_event) {
   // Given
   size_t expected = 0;
   auto target = create_target();

   // When
   auto actual = target.revision();

   // Then
   ASSERT_EQ(expected, actual);
}

}
