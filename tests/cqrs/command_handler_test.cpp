// vim: sw=3 ts=3 expandtab smartindent autoindent cindent
#include "cqrs/generic_command_handler.h"
#include "helpers.h"
#include <gtest/gtest.h>

namespace {
using namespace fakeit;
using namespace cddd::cqrs;

using handler_type = command_handler<fake_command, mock_factory::artifact_store_type>;

class command_handler_test : public ::testing::TestWithParam<bool>,
                             public cddd::cqrs::mock_factory {
public:
   inline std::shared_ptr<handler_type> create_target() {
      auto art_store = create_simple_artifact_store();
      persistance_result = create_commit();
      auto es = create_stub_stream();
      When(Method(artifact_factory_spy, create_test_artifact).Using(entity->id())).Return(entity);
      When(Method(source_spy, has_stream_for).Using(entity->id())).Return(true);
      When(Method(source_spy, get_stream_for).Using(entity->id())).Return(es);

      return std::make_shared<handler_type>(GetParam() ? create_passing_command_validator() : create_failing_command_validator(),
                                            art_store);
   }

   std::shared_ptr<test_artifact> entity = std::make_shared<test_artifact>();
};


TEST_P(command_handler_test, command_with_nil_id_returns_invalid_argument) {
   // Given
   auto target = create_target();
   fake_command c;
   c.id = boost::uuids::nil_uuid();

   // When
   std::error_code actual = target->handle(c);

   // Then
   ASSERT_EQ(make_error_code(std::errc::invalid_argument), actual);
}


TEST_P(command_handler_test, command_with_nil_id_does_not_invoke_command_execution) {
   // Given
   auto target = create_target();
   fake_command c;
   c.id = boost::uuids::nil_uuid();
   bool execute_called = false;
   entity->do_command = [&execute_called]() { execute_called = true; };

   // When
   (void)target->handle(c);

   // Then
   ASSERT_FALSE(execute_called);
   Verify(Method(validator_spy, validate)).Exactly(0_Times);
}


TEST_P(command_handler_test, command_should_execute_on_artifact_if_validation_passes) {
   // Given
   auto target = create_target();
   fake_command c;
   c.id = entity->id();
   bool execute_called = false;
   entity->do_command = [&execute_called]() { execute_called = true; };

   // When
   (void)target->handle(c);

   // Then
   Verify(Method(validator_spy, validate)).Once();
   ASSERT_EQ(GetParam(), execute_called);
}


INSTANTIATE_TEST_CASE_P(failing_validation, command_handler_test, ::testing::Values(false));
INSTANTIATE_TEST_CASE_P(passing_validation, command_handler_test, ::testing::Values(true));

}
