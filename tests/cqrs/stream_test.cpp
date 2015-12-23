// vim: sw=3 ts=3 expandtab smartindent autoindent cindent
#include "cqrs/stream.h"
#include "tests/cqrs/helpers.h"
#include <gtest/gtest.h>

namespace {

using namespace fakeit;
using namespace cddd::cqrs;
using kerchow::picker;


class stream_test : public ::testing::Test,
                    public mock_factory {
public:
};


TEST_F(stream_test, load_given_0_min_revision_will_return_empty_sequence) {
   // Given
   auto target = create_abstract_stream();
   std::size_t min_revision = 0;
   std::size_t max_revision = picker.pick<std::size_t>(1, 10);

   // When
   bool actual = target->load(min_revision, max_revision).empty();

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(stream_test, load_given_min_revision_higher_than_max_revision_will_return_empty_sequence) {
   // Given
   auto target = create_abstract_stream();
   std::size_t min_revision = picker.pick<std::size_t>(11, 20);
   std::size_t max_revision = picker.pick<std::size_t>(1, 10);

   // When
   bool actual = target->load(min_revision, max_revision).empty();

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(stream_test, load_given_valid_min_revision_and_max_revision_will_return_derived_value) {
   // Given
   auto target = create_abstract_stream();
   std::size_t min_revision = picker.pick<std::size_t>(1, 10);
   std::size_t max_revision = picker.pick<std::size_t>(min_revision);
   auto expected = create_event_container();
   When(Method(stream_spy, load).Using(min_revision, max_revision)).Return(expected);

   // When
   std::vector<domain_event_pointer> actual = target->load(min_revision, max_revision);

   // Then
   Verify(Method(stream_spy, load).Using(min_revision, max_revision)).Once();
   ASSERT_EQ(expected, actual);
}


TEST_F(stream_test, load_not_given_revision_will_use_1_and_max) {
   // Given
   auto target = create_abstract_stream();
   std::size_t min_revision = 1;
   std::size_t max_revision = std::numeric_limits<std::size_t>::max();
   When(Method(stream_spy, load).Using(min_revision, max_revision)).Return(std::vector<domain_event_pointer>{});

   // When
   target->load(min_revision, max_revision);

   // Then
   Verify(Method(stream_spy, load).Using(min_revision, max_revision)).Once();
}


TEST_F(stream_test, save_given_event_container_should_invoke_save_sequence_with_same_values) {
   // Given
   auto target = create_abstract_stream();
   std::vector<domain_event_pointer> expected{create_event_container()};
   std::vector<domain_event_pointer> actual;
   When(Method(stream_spy, save)).Do([&actual](auto &s) { actual = std::move(s); });

   // When
   target->save(expected);

   // Then
   ASSERT_EQ(expected, actual);
}


#if 0
TEST_F(stream_test, persist_should_invoke_derived_class_to_persist_changes) {
   // Given
   Mock<test_stream> target;
   When(Method(target, persist_changes)).Return(create_commit());

   // When
   target.get().persist();

   // Then
   Verify(Method(target, persist_changes)).Once();
}


TEST_F(stream_test, persist_should_return_derived_commit) {
   // Given
   Mock<test_stream> target;
   commit expected = create_commit();
   When(Method(target, persist_changes)).Return(expected);

   // When
   commit actual = target.get().persist();

   // Then
   ASSERT_EQ(expected.commit_id(), actual.commit_id());
   ASSERT_EQ(expected.stream_id(), actual.stream_id());
   ASSERT_EQ(expected.stream_revision(), actual.stream_revision());
   ASSERT_EQ(expected.commit_sequence(), actual.commit_sequence());
   ASSERT_EQ(expected.timestamp(), actual.timestamp());
}
#endif

}
