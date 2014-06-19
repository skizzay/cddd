#include "cddd/cqrs/artifact.h"


using namespace cddd::cqrs;
using ::testing::_;
using ::testing::AtLeast;


template<class ArtifactType>
artifact_ptr create_artifact(std::shared_ptr<fake_dispatcher>);

template<class ArtifactType>
event_ptr create_handled_event();


template<class ArtifactType>
class artifact_test_fixture : public ::testing::Test {
public:
   inline artifact_test_fixture() :
      dispatcher(std::make_shared<NiceMock<fake_dispatcher>>()),
      target(nullptr)
   {
   }

   void expect_dispatcher_to_have_event_handlers_registered() {
      EXPECT_CALL(*dispatcher, add_handler(_, _))
         .Times(AtLeast(1));
   }

   std::shared_ptr<NiceMock<fake_dispatcher>> dispatcher;
   std::unique_ptr<ArtifactType> target;
};


TYPED_TEST_P(artifact_test_fixture, constructor_registers_event_handlers) {
   // given
   this->expect_dispatcher_to_have_event_handlers_registered();

   // when
   auto target = create_artifact<TypeParam>(this->dispatcher);

   // then
   ASSERT_TRUE(Mock::VerifyAndClear(this->dispatcher.get()));
}


TYPED_TEST_P(artifact_test_fixture, has_uncommitted_events_is_0_when_newly_constructed) {
   // given
   auto target = create_artifact<TypeParam>(this->dispatcher);

   // when
   bool actual = target->has_uncommitted_events();

   // then
   ASSERT_FALSE(actual);
}


TYPED_TEST_P(artifact_test_fixture, uncommitted_events_is_empty_when_newly_constructed) {
   // given
   auto target = create_artifact<TypeParam>(this->dispatcher);

   // when
   bool actual = target->uncommitted_events().empty();

   // then
   ASSERT_TRUE(actual);
}


TYPED_TEST_P(artifact_test_fixture, revision_is_at_0_when_newly_constructed) {
   // given
   auto target = create_artifact<TypeParam>(this->dispatcher);

   // when
   std::size_t actual = target->revision();

   // then
   ASSERT_EQ(0, actual);
}


template<class ArtifactType>
class artifact_test_with_handled_event : public artifact_test_fixture<ArtifactType> {
public:
   artifact_test_with_handled_event() :
      artifact_test_fixture<ArtifactType>(),
      handled_event(create_handled_event<ArtifactType>()),
      target(create_artifact<ArtifactType>(this->dispatcher))
   {
      target->apply_change(handled_event);
   }

   event_ptr handled_event;
   artifact_ptr target;
};


TYPED_TEST_P(artifact_test_with_handled_event, revision_gets_incremented) {
   // given
   std::size_t expected = 1;

   // when
   std::size_t actual = this->target->revision();

   // then
   ASSERT_EQ(expected, actual);
}


TYPED_TEST_P(artifact_test_with_handled_event, has_uncommitted_events_is_true) {
   // given

   // when
   bool actual = this->target->has_uncommitted_events();

   // then
   ASSERT_TRUE(actual);
}


TYPED_TEST_P(artifact_test_with_handled_event, uncommitted_events_has_another_event) {
   // given
   std::size_t expected = 1;

   // when
   std::size_t actual = this->target->uncommitted_events().count();

   // then
   ASSERT_EQ(expected, actual);
}


template<class ArtifactType>
class artifact_test_with_handled_event_then_cleared : public artifact_test_with_handled_event<ArtifactType> {
public:
   artifact_test_with_handled_event_then_cleared() :
      artifact_test_with_handled_event<ArtifactType>(),
   {
      this->target->clear_uncommitted_events();
   }
};


TYPED_TEST_P(artifact_test_with_handled_event_then_cleared, has_committed_events_is_false) {
   // given

   // when
   bool actual = target->has_uncommitted_events();

   // then
   ASSERT_FALSE(actual);
}


TYPED_TEST_P(artifact_test_with_handled_event_then_cleared, committed_events_is_empty) {
   // given

   // when
   bool actual = target->uncommitted_events().empty();

   // then
   ASSERT_TRUE(actual);
}
