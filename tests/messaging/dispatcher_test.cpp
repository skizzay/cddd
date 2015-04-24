#include "messaging/dispatcher.h"
#include <gtest/gtest.h>

namespace {

using namespace cddd::messaging;
using namespace cddd::utils;


template<class DispatchingTablePolicy>
class dispatcher_tests : public ::testing::Test {
public:
   typedef dispatcher<return_error_code_on_handling_errors, DispatchingTablePolicy> target_type;

   inline auto create_target() {
      return target_type{};
   }
};

TYPED_TEST_CASE_P(dispatcher_tests);


TYPED_TEST_P(dispatcher_tests, add_message_handler_given_a_message_handler_should_return_success) {
   // Arrange
   auto target = this->create_target();
   std::error_code expected{make_error_code(dispatching_error::none)};

   // Act
   std::error_code actual = target.add_message_handler([](int){});

   // Assert
   ASSERT_EQ(expected, actual);
}


TYPED_TEST_P(dispatcher_tests, add_message_handler_given_a_message_handler_with_filter_should_return_success) {
   // Arrange
   auto target = this->create_target();
   std::error_code expected{make_error_code(dispatching_error::none)};

   // Act
   std::error_code actual = target.add_message_handler([](int){},[](int) { return true; });

   // Assert
   ASSERT_EQ(expected, actual);
}


TYPED_TEST_P(dispatcher_tests, add_message_handler_given_a_message_translator_should_return_success) {
   // Arrange
   auto target = this->create_target();
   std::error_code expected{make_error_code(dispatching_error::none)};

   // Act
   std::error_code actual = target.add_message_handler([](int){ return double{}; });

   // Assert
   ASSERT_EQ(expected, actual);
}


TYPED_TEST_P(dispatcher_tests, add_message_handler_given_a_message_translator_with_filter_should_return_success) {
   // Arrange
   auto target = this->create_target();
   std::error_code expected{make_error_code(dispatching_error::none)};

   // Act
   std::error_code actual = target.add_message_handler([](int){ return double{}; }, [](int){ return false; });

   // Assert
   ASSERT_EQ(expected, actual);
}


TYPED_TEST_P(dispatcher_tests, add_message_handler_given_a_message_translator_of_same_translation_should_return_failed_to_add_handler) {
   // Arrange
   auto target = this->create_target();
   std::error_code expected{make_error_code(dispatching_error::failed_to_add_handler)};
   target.add_message_handler([](int){ return double{}; });

   // Act
   std::error_code actual = target.add_message_handler([](int){ return double{}; });

   // Assert
   ASSERT_EQ(expected, actual);
}


TYPED_TEST_P(dispatcher_tests, dispatch_message_given_no_handlers_should_return_no_handlers_found) {
   // Arrange
   auto target = this->create_target();
   std::error_code expected{make_error_code(dispatching_error::no_handlers_found)};

   // Act
   std::error_code actual = target.dispatch_message(std::rand());

   // Assert
   ASSERT_EQ(expected, actual);
}


TYPED_TEST_P(dispatcher_tests, dispatch_message_given_a_message_translator_but_no_handlers_should_return_no_handlers_found) {
   // Arrange
   auto target = this->create_target();
   std::error_code expected{make_error_code(dispatching_error::no_handlers_found)};
   target.add_message_handler([](int){ return double{}; });

   // Act
   std::error_code actual = target.dispatch_message(std::rand());

   // Assert
   ASSERT_EQ(expected, actual);
}


TYPED_TEST_P(dispatcher_tests, dispatch_message_given_single_handler_should_return_success) {
   // Arrange
   auto target = this->create_target();
   std::error_code expected{make_error_code(dispatching_error::none)};
   target.add_message_handler([](int){});

   // Act
   std::error_code actual = target.dispatch_message(std::rand());

   // Assert
   ASSERT_EQ(expected, actual);
}


TYPED_TEST_P(dispatcher_tests, dispatch_message_given_single_handler_with_filter_should_invoke_the_filter) {
   // Arrange
   auto target = this->create_target();
   bool invoked = false;
   target.add_message_handler([](int){}, [&](int) { invoked = true; return true; });

   // Act
   target.dispatch_message(std::rand());

   // Assert
   ASSERT_TRUE(invoked);
}


TYPED_TEST_P(dispatcher_tests, dispatch_message_given_single_handler_with_passing_filter_should_invoke_the_handler) {
   // Arrange
   auto target = this->create_target();
   bool invoked = false;
   target.add_message_handler([&](int){ invoked = true; }, [](int) { return true; });

   // Act
   target.dispatch_message(std::rand());

   // Assert
   ASSERT_TRUE(invoked);
}


TYPED_TEST_P(dispatcher_tests, dispatch_message_given_single_handler_with_failing_filter_should_not_invoke_the_handler) {
   // Arrange
   auto target = this->create_target();
   bool invoked = false;
   target.add_message_handler([&](int){ invoked = true; }, [](int) { return false; });

   // Act
   target.dispatch_message(std::rand());

   // Assert
   ASSERT_FALSE(invoked);
}


TYPED_TEST_P(dispatcher_tests, dispatch_message_given_single_handler_should_invoke_each_handler) {
   // Arrange
   auto target = this->create_target();
   bool invoked = false;
   target.add_message_handler([&](int){ invoked = true; });

   // Act
   target.dispatch_message(std::rand());

   // Assert
   ASSERT_TRUE(invoked);
}


TYPED_TEST_P(dispatcher_tests, dispatch_message_given_translator_and_single_handler_should_invoke_each_handler) {
   // Arrange
   auto target = this->create_target();
   bool invoked = false;
   target.add_message_handler([&](double){ invoked = true; });
   target.add_message_handler([](int i){ return static_cast<double>(i); });

   // Act
   target.dispatch_message(std::rand());

   // Assert
   ASSERT_TRUE(invoked);
}


TYPED_TEST_P(dispatcher_tests, dispatch_message_given_translator_with_filter_and_single_handler_should_invoke_the_filter) {
   // Arrange
   auto target = this->create_target();
   bool invoked = false;
   target.add_message_handler([](double){});
   target.add_message_handler([](int i){ return static_cast<double>(i); }, [&](int) { invoked = true; return true; });

   // Act
   target.dispatch_message(std::rand());

   // Assert
   ASSERT_TRUE(invoked);
}


TYPED_TEST_P(dispatcher_tests, dispatch_message_given_translator_with_a_passing_filter_and_single_handler_should_invoke_translator) {
   // Arrange
   auto target = this->create_target();
   bool invoked = false;
   target.add_message_handler([](double){});
   target.add_message_handler([&](int i){ invoked = true; return static_cast<double>(i); }, [](int) { return true; });

   // Act
   target.dispatch_message(std::rand());

   // Assert
   ASSERT_TRUE(invoked);
}


TYPED_TEST_P(dispatcher_tests, dispatch_message_given_translator_with_a_failing_filter_and_single_handler_should_not_invoke_translator) {
   // Arrange
   auto target = this->create_target();
   bool invoked = false;
   target.add_message_handler([](double){});
   target.add_message_handler([&](int i){ invoked = true; return static_cast<double>(i); }, [](int) { return false; });

   // Act
   target.dispatch_message(std::rand());

   // Assert
   ASSERT_FALSE(invoked);
}


TYPED_TEST_P(dispatcher_tests, dispatch_message_given_translator_and_single_handler_should_return_success) {
   // Arrange
   auto target = this->create_target();
   std::error_code expected{make_error_code(dispatching_error::none)};
   target.add_message_handler([&](double){});
   target.add_message_handler([&](int i){ return static_cast<double>(i); });

   // Act
   std::error_code actual = target.dispatch_message(std::rand());

   // Assert
   ASSERT_EQ(expected, actual);
}


TYPED_TEST_P(dispatcher_tests, dispatch_message_given_multiple_handlers_of_different_types_should_return_success) {
   // Arrange
   auto target = this->create_target();
   std::error_code expected{make_error_code(dispatching_error::none)};
   target.add_message_handler([](int){});
   target.add_message_handler([](double){});

   // Act
   std::error_code actual = target.dispatch_message(std::rand());

   // Assert
   ASSERT_EQ(expected, actual);
}


TYPED_TEST_P(dispatcher_tests, dispatch_message_given_multiple_handlers_of_different_types_should_invoke_correct_handler) {
   // Arrange
   auto target = this->create_target();
   bool invoked1 = false;
   bool invoked2 = false;
   target.add_message_handler([&](int){ invoked1 = true; });
   target.add_message_handler([&](double){ invoked2 = true; });

   // Act
   target.dispatch_message(std::rand());

   // Assert
   ASSERT_TRUE(invoked1);
   ASSERT_FALSE(invoked2);
}


REGISTER_TYPED_TEST_CASE_P(dispatcher_tests,
                           add_message_handler_given_a_message_handler_should_return_success,
                           add_message_handler_given_a_message_handler_with_filter_should_return_success,
                           add_message_handler_given_a_message_translator_should_return_success,
                           add_message_handler_given_a_message_translator_with_filter_should_return_success,
                           add_message_handler_given_a_message_translator_of_same_translation_should_return_failed_to_add_handler,
                           dispatch_message_given_no_handlers_should_return_no_handlers_found,
                           dispatch_message_given_a_message_translator_but_no_handlers_should_return_no_handlers_found,
                           dispatch_message_given_single_handler_should_return_success,
                           dispatch_message_given_single_handler_with_filter_should_invoke_the_filter,
                           dispatch_message_given_single_handler_with_passing_filter_should_invoke_the_handler,
                           dispatch_message_given_single_handler_with_failing_filter_should_not_invoke_the_handler,
                           dispatch_message_given_single_handler_should_invoke_each_handler,
                           dispatch_message_given_translator_and_single_handler_should_invoke_each_handler,
                           dispatch_message_given_translator_and_single_handler_should_return_success,
                           dispatch_message_given_translator_with_filter_and_single_handler_should_invoke_the_filter,
                           dispatch_message_given_translator_with_a_passing_filter_and_single_handler_should_invoke_translator,
                           dispatch_message_given_translator_with_a_failing_filter_and_single_handler_should_not_invoke_translator,
                           dispatch_message_given_multiple_handlers_of_different_types_should_return_success,
                           dispatch_message_given_multiple_handlers_of_different_types_should_invoke_correct_handler);


typedef ::testing::Types<use_multimap, use_map, use_unordered_multimap, use_unordered_map> test_types;
INSTANTIATE_TYPED_TEST_CASE_P(messaging_test_suite, dispatcher_tests, test_types);




template<class DispatchingTablePolicy>
class multiple_handler_dispatcher_tests : public dispatcher_tests<DispatchingTablePolicy> {
public:
};

TYPED_TEST_CASE_P(multiple_handler_dispatcher_tests);


TYPED_TEST_P(multiple_handler_dispatcher_tests, dispatch_message_given_multiple_handlers_of_same_type_should_return_success) {
   // Arrange
   auto target = this->create_target();
   std::error_code expected{make_error_code(dispatching_error::none)};
   target.add_message_handler([](int){});
   target.add_message_handler([](int){});

   // Act
   std::error_code actual = target.dispatch_message(std::rand());

   // Assert
   ASSERT_EQ(expected, actual);
}


TYPED_TEST_P(multiple_handler_dispatcher_tests, dispatch_message_given_multiple_handlers_of_same_type_should_invoke_each_handler) {
   // Arrange
   auto target = this->create_target();
   bool invoked1 = false;
   bool invoked2 = false;
   target.add_message_handler([&](int){ invoked1 = true; });
   target.add_message_handler([&](const int &){ invoked2 = true; });

   // Act
   target.dispatch_message(std::rand());

   // Assert
   ASSERT_TRUE(invoked1);
   ASSERT_TRUE(invoked2);
}

REGISTER_TYPED_TEST_CASE_P(multiple_handler_dispatcher_tests,
                           dispatch_message_given_multiple_handlers_of_same_type_should_invoke_each_handler,
                           dispatch_message_given_multiple_handlers_of_same_type_should_return_success);

typedef ::testing::Types<use_multimap, use_unordered_multimap> multi_test_types;
INSTANTIATE_TYPED_TEST_CASE_P(messaging_test_suite, multiple_handler_dispatcher_tests, multi_test_types);




template<class DispatchingTablePolicy>
class single_handler_dispatcher_tests : public dispatcher_tests<DispatchingTablePolicy> {
public:
};

TYPED_TEST_CASE_P(single_handler_dispatcher_tests);


TYPED_TEST_P(single_handler_dispatcher_tests, add_message_handler_given_multiple_handlers_of_same_type_should_return_failed_to_add_handler) {
   // Arrange
   auto target = this->create_target();
   std::error_code expected{make_error_code(dispatching_error::failed_to_add_handler)};
   target.add_message_handler([](int){});

   // Act
   std::error_code actual = target.add_message_handler([](const int){});

   // Assert
   ASSERT_EQ(expected, actual);
}


TYPED_TEST_P(single_handler_dispatcher_tests, add_message_handler_given_single_handler_and_existing_translator_of_same_type_should_return_success) {
   // Arrange
   auto target = this->create_target();
   std::error_code expected{make_error_code(dispatching_error::none)};
   target.add_message_handler([](int i){ return static_cast<double>(i); });

   // Act
   std::error_code actual = target.add_message_handler([](const int){});

   // Assert
   ASSERT_EQ(expected, actual);
}


TYPED_TEST_P(single_handler_dispatcher_tests, add_message_handler_given_single_translator_and_existing_handler_of_same_type_should_return_success) {
   // Arrange
   auto target = this->create_target();
   std::error_code expected{make_error_code(dispatching_error::none)};
   target.add_message_handler([](const int){});

   // Act
   std::error_code actual = target.add_message_handler([](int i){ return static_cast<double>(i); });

   // Assert
   ASSERT_EQ(expected, actual);
}

REGISTER_TYPED_TEST_CASE_P(single_handler_dispatcher_tests,
                           add_message_handler_given_multiple_handlers_of_same_type_should_return_failed_to_add_handler,
                           add_message_handler_given_single_handler_and_existing_translator_of_same_type_should_return_success,
                           add_message_handler_given_single_translator_and_existing_handler_of_same_type_should_return_success);

typedef ::testing::Types<use_map, use_unordered_map> single_test_types;
INSTANTIATE_TYPED_TEST_CASE_P(messaging_test_suite, single_handler_dispatcher_tests, single_test_types);

}
