// vim: sw=3 ts=3 expandtab smartindent autoindent cindent
#include "cqrs/artifact_store.h"
#include "tests/cqrs/helpers.h"
#include "cqrs/artifact.h"
#include "cqrs/commit.h"
#include <boost/uuid/uuid_io.hpp>
#include <gtest/gtest.h>
#include <deque>


namespace {

using namespace fakeit;
using namespace cddd::cqrs;


class artifact_store_test : public ::testing::Test,
                            public mock_factory {
public:
   std::shared_ptr<test_artifact> create_entity() {
      return std::shared_ptr<test_artifact>{&entity, dont_delete};
   }

   test_artifact entity;
};


TEST_F(artifact_store_test, has_asserts_false_when_object_id_is_null) {
   // Given
   auto target = create_simple_artifact_store();

   // When and Then
   ASSERT_DEATH(target->has(boost::uuids::nil_uuid()), "");
}


TEST_F(artifact_store_test, has_returns_false_when_events_provider_does_not_have_artifact) {
   // Given
   auto target = create_simple_artifact_store();
   auto id = gen_id();
   When(Method(source_spy, has_stream_for).Using(id)).Return(false);

   // When
   bool actual = target->has(id);

   // Then
   ASSERT_FALSE(actual);
   Verify(Method(source_spy, has_stream_for).Using(id)).Once();
}


TEST_F(artifact_store_test, has_returns_true_when_events_provider_does_have_artifact) {
   // Given
   auto target = create_simple_artifact_store();
   auto id = gen_id();
   When(Method(source_spy, has_stream_for).Using(id)).Return(true);

   // When
   bool actual = target->has(id);

   // Then
   ASSERT_TRUE(actual);
   Verify(Method(source_spy, has_stream_for).Using(id)).Once();
}


TEST_F(artifact_store_test, put_will_not_save_the_object_when_the_object_has_no_uncommitted_events) {
   // Given
   auto target = create_simple_artifact_store();

   // When
   target->put(entity);

   // Then
   VerifyNoOtherInvocations(source_spy);
}


TEST_F(artifact_store_test, put_will_return_noncommit_when_the_object_has_no_uncommitted_events) {
   // Given
   auto target = create_simple_artifact_store();

   // When
   auto actual = target->put(entity);

   // Then
   ASSERT_TRUE(actual.is_noncommit());
}


TEST_F(artifact_store_test, put_will_save_the_object_when_the_object_has_uncommitted_events) {
   // Given
   auto target = create_simple_artifact_store();
   persistance_result = create_commit();
   auto es = create_stub_stream();
   When(Method(source_spy, has_stream_for).Using(entity.id())).Return(true);
   When(Method(source_spy, get_stream_for).Using(entity.id())).Return(es);
   entity.apply_change(fake_event{});

   // When
   commit actual = target->put(entity);

   // Then
   ASSERT_FALSE(actual.is_noncommit());
   ASSERT_EQ(persistance_result.commit_id(), actual.commit_id());
   ASSERT_EQ(persistance_result.stream_id(), actual.stream_id());
   ASSERT_EQ(persistance_result.stream_revision(), actual.stream_revision());
   ASSERT_EQ(persistance_result.timestamp(), actual.timestamp());
}


TEST_F(artifact_store_test, get_will_assert_false_when_object_id_is_null) {
   // Given
   auto target = create_simple_artifact_store();
   size_t version = kerchow::picker.pick<size_t>();

   // When
   ASSERT_DEATH(target->get(boost::uuids::nil_uuid(), version), "");
}


TEST_F(artifact_store_test, get_will_load_event_sequence_for_all_events) {
   // Given
   auto target = create_simple_artifact_store();
   boost::uuids::uuid id = entity.id();
   size_t version = std::numeric_limits<size_t>::max();
   load_playback = create_event_container();
   auto es = create_stub_stream();
   auto entity_pointer = create_entity();
   entity_pointer->handle([](const fake_event &){});
   When(Method(artifact_factory_spy, create_test_artifact).Using(id)).Return(entity_pointer);
   When(Method(source_spy, has_stream_for).Using(id)).Return(false);
   When(Method(source_spy, create_stream_for).Using(id)).Return(es);

   // When
   auto actual = target->get(id, version);

   // Then
   ASSERT_EQ(entity_pointer, actual);
   ASSERT_EQ(load_playback.size(), actual->revision());
}


TEST_F(artifact_store_test, get_with_revision_equal_to_requested_version_due_to_memento_loading_will_not_load_events) {
   // Given
   auto target = create_simple_artifact_store();
   boost::uuids::uuid id = entity.id();
   size_t version = kerchow::picker.pick<size_t>(1);
   auto entity_pointer = create_entity();
   When(Method(artifact_factory_spy, create_test_artifact).Using(id)).Return(entity_pointer);
   entity.set_version(version);

   // When
   target->get(id, version);

   // Then
   Verify(Method(source_spy, has_stream_for)).Exactly(0_Times);
}

}
