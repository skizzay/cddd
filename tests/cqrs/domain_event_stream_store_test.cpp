// vim: sw=3 ts=3 expandtab smartindent autoindent cindent
#include "cqrs/domain_event_stream_store.h"
#include <range/v3/empty.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <fakeit.hpp>
#include <gtest/gtest.h>
#include <kerchow/kerchow.h>

namespace {

using namespace fakeit;
using boost::uuids::nil_uuid;
using namespace cddd::cqrs;

auto dont_delete = [](auto *){};

boost::uuids::basic_random_generator<decltype(kerchow::picker)> gen_id{kerchow::picker};


class stream_implementation final {
public:
   inline auto load(std::size_t, std::size_t) const noexcept {
      return ranges::view::empty<int>();
   }

   template<class T>
   inline void save(T) noexcept {
   }
};


class store_implementation {
public:
   virtual ~store_implementation() noexcept = default;

   virtual bool has_stream_for(const boost::uuids::uuid &) const = 0;
   virtual std::shared_ptr<stream<stream_implementation>> get_stream_for(const boost::uuids::uuid &) const = 0;
   virtual std::shared_ptr<stream<stream_implementation>> create_stream_for(const boost::uuids::uuid &) const = 0;
};


class domain_event_stream_store_test : public ::testing::Test {
public:
   inline auto create_target() {
      std::unique_ptr<store_implementation, decltype(dont_delete)> spy{&store_spy.get(), dont_delete};
      return domain_event_stream_store<store_implementation, decltype(dont_delete)> {std::move(spy)};
   }

   inline auto create_stream() {
      return std::make_shared<stream<stream_implementation>>(std::make_unique<stream_implementation>());
   }

   Mock<store_implementation> store_spy;
};


TEST_F(domain_event_stream_store_test, has_given_null_uuid_should_return_false) {
   // Given
   auto target = create_target();
   auto id = nil_uuid();

   // When and Then
   ASSERT_DEATH(target.has(id), "");
}


TEST_F(domain_event_stream_store_test, has_given_valid_uuid_and_stream_exists_should_return_true) {
   // Given
   auto target = create_target();
   auto id = gen_id();
   When(Method(store_spy, has_stream_for).Using(id)).Return(true);

   // When
   bool actual = target.has(id);

   // Then
   ASSERT_TRUE(actual);
   Verify(Method(store_spy, has_stream_for).Using(id)).Once();
}


TEST_F(domain_event_stream_store_test, has_given_valid_uuid_and_stream_does_not_exist_should_return_false) {
   // Given
   auto target = create_target();
   auto id = gen_id();
   When(Method(store_spy, has_stream_for).Using(id)).Return(false);

   // When
   bool actual = target.has(id);

   // Then
   ASSERT_FALSE(actual);
   Verify(Method(store_spy, has_stream_for).Using(id)).Once();
}


TEST_F(domain_event_stream_store_test, get_or_create_given_valid_uuid_and_stream_does_not_exist_should_create_stream) {
   // Given
   auto target = create_target();
   auto id = gen_id();
   auto expected = create_stream();
   When(Method(store_spy, has_stream_for).Using(id)).Return(false);
   When(Method(store_spy, create_stream_for).Using(id)).Return(expected);

   // When
   std::shared_ptr<stream<stream_implementation>> actual = target.get_or_create(id);

   // Then
   ASSERT_EQ(expected, actual);
}


TEST_F(domain_event_stream_store_test, get_or_create_given_valid_uuid_and_stream_does_exist_should_get_stream) {
   // Given
   auto target = create_target();
   auto id = gen_id();
   auto expected = create_stream();
   When(Method(store_spy, has_stream_for).Using(id)).Return(true);
   When(Method(store_spy, get_stream_for).Using(id)).Return(expected);

   // When
   std::shared_ptr<stream<stream_implementation>> actual = target.get_or_create(id);

   // Then
   ASSERT_EQ(expected, actual);
}

}
