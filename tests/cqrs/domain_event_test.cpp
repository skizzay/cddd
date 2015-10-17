#include "cqrs/domain_event.h"
#include "cqrs/fakes/fake_event.h"
#include <gtest/gtest.h>

namespace {

using namespace cddd::cqrs;


TEST(domain_event_test, is_event_when_using_the_same_event_type_should_return_true) {
   // Given
   std::unique_ptr<domain_event> target{new basic_domain_event<fake_event>{{}, 1}};

   // When
   bool actual = is_event<fake_event>(*target);

   // Then
   ASSERT_TRUE(actual);
}


TEST(domain_event_test, is_event_when_using_the_different_event_type_should_return_false) {
   // Given
   std::unique_ptr<domain_event> target{new basic_domain_event<fake_event>{{}, 1}};

   // When
   bool actual = is_event<int>(*target);

   // Then
   ASSERT_FALSE(actual);
}


TEST(domain_event_test, unsafe_event_cast_when_using_the_same_event_type_should_cast_the_event) {
   // Given
   std::unique_ptr<domain_event> target{new basic_domain_event<fake_event>{{}, 1}};

   // When
   ASSERT_NO_THROW(unsafe_event_cast<fake_event>(*target));
}


TEST(domain_event_test, safe_event_cast_when_using_the_different_event_type_should_throw_bad_cast) {
   // Given
   std::unique_ptr<domain_event> target{new basic_domain_event<fake_event>{{}, 1}};

   // When
   ASSERT_THROW(safe_event_cast<int>(*target), std::bad_cast);
}

}
