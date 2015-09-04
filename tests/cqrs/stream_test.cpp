#include "cqrs/stream.h"
#include <fakeit.hpp>
#include <gtest/gtest.h>
#include <kerchow/kerchow.h>
#include <boost/uuid/uuid_generators.hpp>

namespace {

using namespace fakeit;
using namespace sequencing;
using namespace cddd::cqrs;
using kerchow::picker;

boost::uuids::basic_random_generator<decltype(picker)> gen_id{picker};


class test_stream : public stream<test_stream> {
public:
   virtual ~test_stream() noexcept = default;

   virtual std::vector<int> load_revisions(std::size_t min_revision, std::size_t max_revision) const = 0;
   virtual void save_sequence(sequence<int> &&s) = 0;
   virtual commit persist_changes() = 0;
};


class stream_test : public ::testing::Test {
public:
   inline std::vector<int> create_event_container() const {
      std::size_t num_values = picker.pick<std::size_t>(1, 30);
      std::vector<int> event_container(num_values);

      for (int &event : event_container) {
         event = picker.pick<std::size_t>();
      }

      return std::move(event_container);
   }

   inline commit create_commit() const {
      return commit{gen_id(), gen_id(), picker.pick<std::size_t>(1), picker.pick<std::size_t>(1),
                    boost::posix_time::microsec_clock::universal_time()};
   }
};


TEST_F(stream_test, load_given_0_min_revision_will_return_empty_sequence) {
   // Given
   Mock<test_stream> target;
   std::size_t min_revision = 0;
   std::size_t max_revision = picker.pick<std::size_t>(1, 10);

   // When
   bool actual = target.get().load(min_revision, max_revision).empty();

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(stream_test, load_given_min_revision_higher_than_max_revision_will_return_empty_sequence) {
   // Given
   Mock<test_stream> target;
   std::size_t min_revision = picker.pick<std::size_t>(11, 20);
   std::size_t max_revision = picker.pick<std::size_t>(1, 10);

   // When
   bool actual = target.get().load(min_revision, max_revision).empty();

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(stream_test, load_given_valid_min_revision_and_max_revision_will_return_derived_value) {
   // Given
   Mock<test_stream> target;
   std::size_t min_revision = picker.pick<std::size_t>(1, 10);
   std::size_t max_revision = picker.pick<std::size_t>(min_revision);
   When(Method(target, load_revisions).Using(min_revision, max_revision)).Return(std::vector<int>{});

   // When
   target.get().load(min_revision, max_revision);

   // Then
   Verify(Method(target, load_revisions).Using(min_revision, max_revision)).Once();
}


TEST_F(stream_test, save_given_event_container_should_invoke_save_sequence_with_same_values) {
   // Given
   Mock<test_stream> target;
   std::vector<int> event_container{create_event_container()};
   sequence<int> actual;
   When(Method(target, save_sequence)).Do([&actual](sequence<int> &s) { actual = std::move(s); });

   // When
   target.get().save(event_container);

   // Then
   std::equal(std::begin(actual), std::end(actual), std::begin(event_container));
}


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

}
