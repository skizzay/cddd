// vim: sw=3 ts=3 expandtab smartindent autoindent cindent
#include "cqrs/domain_event_stream_store.h"
#include "tests/cqrs/helpers.h"
#include <gtest/gtest.h>

namespace {

using namespace fakeit;
using boost::uuids::nil_uuid;
using namespace cddd::cqrs;


class domain_event_stream_store_test : public ::testing::Test,
                                       public mock_factory {
public:
};


TEST_F(domain_event_stream_store_test, has_given_null_uuid_should_return_false) {
   // Given
   auto target = create_abstract_source();
   auto id = nil_uuid();

   // When and Then
   ASSERT_DEATH(target->has(id), "");
}


TEST_F(domain_event_stream_store_test, has_given_valid_uuid_and_stream_exists_should_return_true) {
   // Given
   auto target = create_abstract_source();
   auto id = gen_id();
   When(Method(source_spy, has_stream_for).Using(id)).Return(true);

   // When
   bool actual = target->has(id);

   // Then
   ASSERT_TRUE(actual);
   Verify(Method(source_spy, has_stream_for).Using(id)).Once();
}


TEST_F(domain_event_stream_store_test, has_given_valid_uuid_and_stream_does_not_exist_should_return_false) {
   // Given
   auto target = create_abstract_source();
   auto id = gen_id();
   When(Method(source_spy, has_stream_for).Using(id)).Return(false);

   // When
   bool actual = target->has(id);

   // Then
   ASSERT_FALSE(actual);
   Verify(Method(source_spy, has_stream_for).Using(id)).Once();
}


TEST_F(domain_event_stream_store_test, get_or_create_given_valid_uuid_and_stream_does_not_exist_should_create_stream) {
   // Given
   auto target = create_abstract_source();
   auto id = gen_id();
   auto expected = create_stub_stream();
   When(Method(source_spy, has_stream_for).Using(id)).Return(false);
   When(Method(source_spy, create_stream_for).Using(id)).Return(expected);

   // When
   std::shared_ptr<stream<stream_stub_implementation>> actual = target->get_or_create(id);

   // Then
   ASSERT_EQ(expected, actual);
}


TEST_F(domain_event_stream_store_test, get_or_create_given_valid_uuid_and_stream_does_exist_should_get_stream) {
   // Given
   auto target = create_abstract_source();
   auto id = gen_id();
   auto expected = create_stub_stream();
   When(Method(source_spy, has_stream_for).Using(id)).Return(true);
   When(Method(source_spy, get_stream_for).Using(id)).Return(expected);

   // When
   std::shared_ptr<stream<stream_stub_implementation>> actual = target->get_or_create(id);

   // Then
   ASSERT_EQ(expected, actual);
}

}
