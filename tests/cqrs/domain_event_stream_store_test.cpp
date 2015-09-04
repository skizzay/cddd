#include "cqrs/domain_event_stream_store.h"
#include <boost/uuid/uuid_generators.hpp>
#include <fakeit.hpp>
#include <gtest/gtest.h>
#include <kerchow/kerchow.h>

namespace {

using namespace fakeit;
using boost::uuids::nil_uuid;
using namespace cddd::cqrs;

boost::uuids::basic_random_generator<decltype(kerchow::picker)> gen_id{kerchow::picker};


class test_domain_event_stream_store : public domain_event_stream_store<test_domain_event_stream_store> {
public:
   virtual ~test_domain_event_stream_store() noexcept = default;
   virtual bool has_stream_for(const boost::uuids::uuid &) const = 0;
   virtual void* get_stream_for(const boost::uuids::uuid &) = 0;
   virtual void* create_stream_for(const boost::uuids::uuid &) = 0;
};


class domain_event_stream_store_test : public ::testing::Test {
public:
};


TEST_F(domain_event_stream_store_test, has_given_null_uuid_should_return_false) {
   // Given
   auto id = nil_uuid();
   Mock<test_domain_event_stream_store> target;

   // When
   bool actual = target.get().has(id);

   // Then
   ASSERT_FALSE(actual);
   VerifyNoOtherInvocations(Method(target, has_stream_for));
}


TEST_F(domain_event_stream_store_test, has_given_valid_uuid_and_stream_exists_should_return_true) {
   // Given
   auto id = gen_id();
   Mock<test_domain_event_stream_store> target;
   When(Method(target, has_stream_for).Using(id)).Return(true);

   // When
   bool actual = target.get().has(id);

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(domain_event_stream_store_test, has_given_valid_uuid_and_stream_does_not_exist_should_return_false) {
   // Given
   auto id = gen_id();
   Mock<test_domain_event_stream_store> target;
   When(Method(target, has_stream_for).Using(id)).Return(false);

   // When
   bool actual = target.get().has(id);

   // Then
   ASSERT_FALSE(actual);
}

}
