#include "messaging/dispatcher.h"
#include <gtest/gtest.h>
#include <cucumber-cpp/defs.hpp>


using cucumber::ScenarioScope;
using namespace cddd::messaging;


namespace {

class dummy_command_message {
public:
   dummy_command_message() = default;

   dummy_command_message(std::string s, int i) :
      string_value{std::move(s)},
      int_value{i}
   {
   }

   std::string string_value;
   int int_value = 0;
};


class active_command {
public:
   active_command() = default;

   void create_dispatcher() {
      target = std::make_unique<dispatcher<>>();

      target->add_message_handler([](const std::string &message) {
            std::istringstream iss(message);
            std::string s;
            int i = 0;
            iss >> s >> i;
            return dummy_command_message{std::move(s), i};
         });
   }

   dummy_command_message last_command;
   std::unique_ptr<dispatcher<>> target;
};


GIVEN("^a message dispatcher$") {
   ScenarioScope<active_command> context;
   context->create_dispatcher();
   context->target->add_message_handler([context](const dummy_command_message &command) mutable {
         context->last_command = command;
      });
}


WHEN("^I send a dummy command message: \"([^\\\"]+)\"$") {
   REGEX_PARAM(std::string, message);
   ScenarioScope<active_command> context;

   context->target->dispatch_message(std::string{message});
}


THEN("^the last command should be: (\\w+) (\\d+)$") {
   REGEX_PARAM(std::string, s);
   REGEX_PARAM(int, i);
   ScenarioScope<active_command> context;

   ASSERT_EQ(s, context->last_command.string_value);
   ASSERT_EQ(i, context->last_command.int_value);
}

}
