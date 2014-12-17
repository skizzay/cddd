#include "messaging/router.h"
#include <gtest/gtest.h>

namespace {

using namespace cddd::messaging;


struct error_policy {
   typedef void result_type;
   static bool invoked;
   static message_type_id id_found;

   template<class MessageType>
   static inline result_type no_routes_found(message_type_id id, const MessageType &) {
      return track_invocation(id);
   }

   static inline result_type no_translator_found(message_type_id id) {
      return track_invocation(id);
   }

   static inline result_type translator_exists(message_type_id id) {
      return track_invocation(id);
   }

   static inline result_type track_invocation(message_type_id id) {
      invoked = true;
      id_found = id;
   }

   static inline void success() {}
};

bool error_policy::invoked = false;
message_type_id error_policy::id_found = 0;


class router_tests : public ::testing::Test {
public:
   typedef basic_router<error_policy> target_type;

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


TEST_F(router_tests, add_translator_no_translators_should_not_invoke_error_policy) {
   // Arrange
   auto target = create_target();

   // Act
   target.add_translator([](const void *, std::size_t) { return std::rand(); }, [](const void*, std::size_t) { return true; });

   // Assert
   ASSERT_FALSE(error_policy::invoked);
}


TEST_F(router_tests, add_translator_different_translator_should_not_invoke_error_policy) {
   // Arrange
   auto target = create_target();
   target.add_translator([](const void *, std::size_t) { return int{}; }, [](const void*, std::size_t) { return true; });

   // Act
   target.add_translator([](const void *, std::size_t) { return double{}; }, [](const void*, std::size_t) { return true; });

   // Assert
   ASSERT_FALSE(error_policy::invoked);
}


TEST_F(router_tests, add_translator_with_existing_translator_should_invoke_error_policy) {
   // Arrange
   auto target = create_target();
   target.add_translator([](const void *, std::size_t) { return std::rand(); }, [](const void*, std::size_t) { return true; });

   // Act
   target.add_translator([](const void *, std::size_t) { return std::rand(); }, [](const void*, std::size_t) { return true; });

   // Assert
   ASSERT_TRUE(error_policy::invoked);
   ASSERT_NE(std::size_t{0}, error_policy::id_found);
}


TEST_F(router_tests, add_route_with_no_translators_should_invoke_error_policy) {
   // Arrange
   auto target = create_target();

   // Act
   target.add_route([](double){});

   // Assert
   ASSERT_TRUE(error_policy::invoked);
   ASSERT_NE(std::size_t{0}, error_policy::id_found);
}


TEST_F(router_tests, add_route_with_no_translators_for_message_type_should_invoke_error_policy) {
   // Arrange
   auto target = create_target();
   target.add_translator([](const void *, std::size_t) { return std::rand(); }, [](const void*, std::size_t) { return true; });

   // Act
   target.add_route([](double){});

   // Assert
   ASSERT_TRUE(error_policy::invoked);
   ASSERT_NE(std::size_t{0}, error_policy::id_found);
}


TEST_F(router_tests, add_route_with_translator_for_message_type_should_not_invoke_error_policy) {
   // Arrange
   auto target = create_target();
   target.add_translator([](const void *, std::size_t) { return double{}; }, [](const void*, std::size_t) { return true; });

   // Act
   target.add_route([](double){});

   // Assert
   ASSERT_FALSE(error_policy::invoked);
}


TEST_F(router_tests, on_data_with_no_translators_provided_should_invoke_error_policy) {
   // Arrange
   auto target = create_target();
   auto message = std::rand();

   // Act
   target.on_data(&message, sizeof(message));

   // Assert
   ASSERT_TRUE(error_policy::invoked);
   ASSERT_EQ(std::size_t{0}, error_policy::id_found);
}


TEST_F(router_tests, on_data_with_no_routes_provided_should_invoke_error_policy) {
   // Arrange
   auto target = create_target();
   auto message = std::rand();
   target.add_translator([](const void *data, std::size_t) { return *static_cast<const int*>(data); }, [](const void*, std::size_t) { return true; });

   // Act
   target.on_data(&message, sizeof(message));

   // Assert
   ASSERT_TRUE(error_policy::invoked);
   ASSERT_NE(std::size_t{0}, error_policy::id_found);
}


TEST_F(router_tests, on_data_with_translator_for_message_type_should_not_invoke_error_policy) {
   // Arrange
   auto target = create_target();
   int message = std::rand();
   bool invoked = false;
   target.add_translator([](const void *data, std::size_t) { return *static_cast<const int*>(data); }, [](const void*, std::size_t) { return true; });
   target.add_route([&](int){ invoked = true; });

   // Act
   target.on_data(&message, sizeof(message));

   // Assert
   ASSERT_FALSE(error_policy::invoked);
}


TEST_F(router_tests, on_data_with_translator_for_message_type_should_invoke_route) {
   // Arrange
   auto target = create_target();
   int message = std::rand();
   bool invoked = false;
   target.add_translator([](const void *data, std::size_t) { return *static_cast<const int*>(data); }, [](const void*, std::size_t) { return true; });
   target.add_route([&](int){ invoked = true; });

   // Act
   target.on_data(&message, sizeof(message));

   // Assert
   ASSERT_TRUE(invoked);
}


TEST_F(router_tests, on_data_with_translator_for_message_type_should_pass_message_to_route) {
   // Arrange
   auto target = create_target();
   int message = std::rand();
   int actual = message + 1; // to guarantee they aren't the same value
   target.add_translator([](const void *data, std::size_t) { return *static_cast<const int*>(data); }, [](const void*, std::size_t) { return true; });
   target.add_route([&](int data){ actual = data; });

   // Act
   target.on_data(&message, sizeof(message));

   // Assert
   ASSERT_EQ(message, actual);
}


TEST_F(router_tests, on_data_with_translators_and_routes_for_different_message_types_should_invoke_correct_route) {
   // Arrange
   auto target = create_target();
   int message = std::rand();
   bool i_invoked = false;
   bool d_invoked = false;
   target.add_translator([](const void *data, std::size_t) { return *static_cast<const int*>(data); }, [](const void*, std::size_t) { return true; });
   target.add_translator([](const void *data, std::size_t) { return *static_cast<const double*>(data); }, [](const void*, std::size_t) { return false; });
   target.add_route([&](int){ i_invoked = true; });
   target.add_route([&](double){ d_invoked = true; });

   // Act
   target.on_data(&message, sizeof(message));

   // Assert
   ASSERT_FALSE(d_invoked);
}


TEST_F(router_tests, on_data_with_translators_with_multiple_routes_should_invoke_each_route) {
   // Arrange
   auto target = create_target();
   int message = std::rand();
   bool i1_invoked = false;
   bool i2_invoked = false;
   target.add_translator([](const void *data, std::size_t) { return *static_cast<const int*>(data); }, [](const void*, std::size_t) { return true; });
   target.add_translator([](const void *data, std::size_t) { return *static_cast<const double*>(data); }, [](const void*, std::size_t) { return false; });
   target.add_route([&](int){ i1_invoked = true; });
   target.add_route([&](int){ i2_invoked = true; });

   // Act
   target.on_data(&message, sizeof(message));

   // Assert
   ASSERT_TRUE(i1_invoked);
   ASSERT_TRUE(i2_invoked);
}


TEST_F(router_tests, on_data_with_registered_filered_callback_should_invoke_the_filter) {
   // Arrange
   auto target = create_target();
   int message = std::rand();
   bool invoked = false;
   target.add_translator([](const void *data, std::size_t) { return *static_cast<const int*>(data); }, [](const void*, std::size_t) { return true; });
   target.add_route([](int){}, [&](int) { invoked = true; return false; });

   // Act
   target.on_data(&message, sizeof(message));

   // Assert
   ASSERT_TRUE(invoked);
}


TEST_F(router_tests, on_data_with_registered_filered_callback_should_invoke_the_callback_when_the_filter_passes) {
   // Arrange
   auto target = create_target();
   int message = std::rand();
   bool invoked = false;
   target.add_translator([](const void *data, std::size_t) { return *static_cast<const int*>(data); }, [](const void*, std::size_t) { return true; });
   target.add_route([&](int){ invoked = true; }, [](int) { return true; });

   // Act
   target.on_data(&message, sizeof(message));

   // Assert
   ASSERT_TRUE(invoked);
}


TEST_F(router_tests, on_data_with_registered_filered_callback_should_not_invoke_the_callback_when_the_filter_fails) {
   // Arrange
   auto target = create_target();
   int message = std::rand();
   bool invoked = false;
   target.add_translator([](const void *data, std::size_t) { return *static_cast<const int*>(data); }, [](const void*, std::size_t) { return true; });
   target.add_route([&](int){ invoked = true; }, [](int) { return false; });

   // Act
   target.on_data(&message, sizeof(message));

   // Assert
   ASSERT_FALSE(invoked);
}

}
