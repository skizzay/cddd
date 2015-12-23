// vim: sw=3 ts=3 expandtab smartindent autoindent cindent
#include "cqrs/stream.h"
#include <range/v3/view.hpp>
#include <fakeit.hpp>
#include <gtest/gtest.h>
#include <kerchow/kerchow.h>
#include <boost/uuid/uuid_generators.hpp>

namespace {

using namespace fakeit;
using namespace cddd::cqrs;
using kerchow::picker;
using container_type = decltype(picker.create_fuzzy_container<int>());
using save_range_type = ranges::iterator_range<std::vector<int>::const_iterator, std::vector<int>::const_iterator>;

auto dont_delete = [](auto *){};

boost::uuids::basic_random_generator<decltype(picker)> gen_id{picker};


class stream_implementation {
public:
   virtual ~stream_implementation() noexcept = default;

   virtual std::vector<int> load(size_t min_revision, size_t max_revision) const = 0;
   virtual void save(const save_range_type &container) = 0;
};


class stream_test : public ::testing::Test {
public:
   inline std::vector<int> create_event_container() const {
      return ranges::view::closed_ints(1, picker.pick(1, 30));
   }

   inline auto create_target() {
      std::unique_ptr<stream_implementation, decltype(dont_delete)> spy{&stream_spy.get(), dont_delete};
      return stream<stream_implementation, decltype(dont_delete)> {std::move(spy)};
   }

   Mock<stream_implementation> stream_spy;
};


TEST_F(stream_test, load_given_0_min_revision_will_return_empty_sequence) {
   // Given
   auto target = create_target();
   std::size_t min_revision = 0;
   std::size_t max_revision = picker.pick<std::size_t>(1, 10);

   // When
   bool actual = target.load(min_revision, max_revision).empty();

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(stream_test, load_given_min_revision_higher_than_max_revision_will_return_empty_sequence) {
   // Given
   auto target = create_target();
   std::size_t min_revision = picker.pick<std::size_t>(11, 20);
   std::size_t max_revision = picker.pick<std::size_t>(1, 10);

   // When
   bool actual = target.load(min_revision, max_revision).empty();

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(stream_test, load_given_valid_min_revision_and_max_revision_will_return_derived_value) {
   // Given
   auto target = create_target();
   std::size_t min_revision = picker.pick<std::size_t>(1, 10);
   std::size_t max_revision = picker.pick<std::size_t>(min_revision);
   auto expected = create_event_container();
   When(Method(stream_spy, load).Using(min_revision, max_revision)).Return(expected);

   // When
   std::vector<int> actual = target.load(min_revision, max_revision);

   // Then
   Verify(Method(stream_spy, load).Using(min_revision, max_revision)).Once();
   ASSERT_EQ(expected, actual);
}


TEST_F(stream_test, load_not_given_revision_will_use_1_and_max) {
   // Given
   auto target = create_target();
   std::size_t min_revision = 1;
   std::size_t max_revision = std::numeric_limits<std::size_t>::max();
   When(Method(stream_spy, load).Using(min_revision, max_revision)).Return(std::vector<int>{});

   // When
   target.load(min_revision, max_revision);

   // Then
   Verify(Method(stream_spy, load).Using(min_revision, max_revision)).Once();
}


TEST_F(stream_test, save_given_event_container_should_invoke_save_sequence_with_same_values) {
   // Given
   auto target = create_target();
   std::vector<int> expected{create_event_container()};
   std::vector<int> actual;
   When(Method(stream_spy, save)).Do([&actual](auto &s) { actual = std::move(s); });

   // When
   target.save(expected);

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
