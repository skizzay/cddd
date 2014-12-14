#include "messaging/router.h"
#include <gtest/gtest.h>

namespace {

using namespace cddd::messaging;


struct error_policy {
   static bool invoked;
   static message_type_id id_found;

   template<class MessageType>
   static inline void no_routes_found(message_type_id id, const MessageType &) {
      invoked = true;
      id_found = id;
   }
};

bool error_policy::invoked = false;
message_type_id error_policy::id_found = 0;


class router_tests : public ::testing::Test {
public:
   typedef router<error_policy> target_type;

   router_tests() :
      ::testing::Test()
   {
      error_policy::invoked = false;
      error_policy::id_found = 0;
   }

   inline auto create_target() {
      return target_type{};
   }
};


TEST_F(router_tests, route_with_no_routes_provided_should_invoke_error_policy) {
   // Arrange
   auto target = create_target();

   // Act
   target.route(std::rand());

   // Assert
   ASSERT_TRUE(error_policy::invoked);
   ASSERT_NE(std::size_t{0}, error_policy::id_found);
}


TEST_F(router_tests, route_with_no_routes_of_the_type_provided_should_invoke_error_policy) {
   // Arrange
   auto target = create_target();
   target.add_route([](double){});

   // Act
   target.route(std::rand());

   // Assert
   ASSERT_TRUE(error_policy::invoked);
   ASSERT_NE(std::size_t{0}, error_policy::id_found);
}


TEST_F(router_tests, route_with_no_routes_of_the_type_provided_should_not_invoke_callbacks) {
   // Arrange
   auto target = create_target();
   bool invoked = false;
   target.add_route([&](double){ invoked = true; });

   // Act
   target.route(std::rand());

   // Assert
   ASSERT_FALSE(invoked);
}


TEST_F(router_tests, route_with_registered_callback_should_invoke_each_callback) {
   // Arrange
   int actual = std::numeric_limits<int>::max();
   int expected = std::rand();
   while (expected == actual) { expected = std::rand(); }
   auto f = [&](int value) { actual = value; };
   auto target = create_target();
   target.add_route(f);

   // Act
   target.route(expected);

   // Assert
   ASSERT_EQ(expected, actual);
}


TEST_F(router_tests, route_with_multiple_registered_callbacks_should_invoke_each_callback) {
   // Arrange
   bool f_invoked = false;
   bool g_invoked = false;
   auto f = [&](int) { f_invoked = true; };
   auto g = [&](int) { g_invoked = true; };
   auto target = create_target();
   target.add_route(f);
   target.add_route(g);

   // Act
   target.route(std::rand());

   // Assert
   ASSERT_TRUE(f_invoked);
   ASSERT_TRUE(g_invoked);
}


TEST_F(router_tests, route_with_registered_callbacks_for_different_types_should_invoke_only_callback_for_specified_type) {
   // Arrange
   bool f_invoked = false;
   bool g_invoked = false;
   auto f = [&](int) { f_invoked = true; };
   auto g = [&](double) { g_invoked = true; };
   auto target = create_target();
   target.add_route(f);
   target.add_route(g);

   // Act
   target.route(std::rand());

   // Assert
   ASSERT_TRUE(f_invoked);
   ASSERT_FALSE(g_invoked);
}


TEST_F(router_tests, route_with_registered_filered_callback_should_invoke_the_filter) {
   // Arrange
   bool invoked = false;
   auto target = create_target();
   target.add_route([](int){}, [&](int) { invoked = true; return false; });

   // Act
   target.route(std::rand());

   // Assert
   ASSERT_TRUE(invoked);
}


TEST_F(router_tests, route_with_registered_filered_callback_should_invoke_the_callback_when_the_filter_passes) {
   // Arrange
   bool f_invoked = false;
   auto f = [&](int) { f_invoked = true; };
   auto target = create_target();
   target.add_route(f, [](int) { return true; });

   // Act
   target.route(std::rand());

   // Assert
   ASSERT_TRUE(f_invoked);
}


TEST_F(router_tests, route_with_registered_filered_callback_should_not_invoke_the_callback_when_the_filter_fails) {
   // Arrange
   bool f_invoked = false;
   auto f = [&](const int &) { f_invoked = true; };
   auto target = create_target();
   target.add_route(f, [](int) { return false; });

   // Act
   target.route(std::rand());

   // Assert
   ASSERT_FALSE(f_invoked);
}

}
