#include "cqrs/artifact.h"
#include "kerchow/kerchow.h"
#include <boost/uuid/random_generator.hpp>
#include <cassert>
#include <gtest/gtest.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <cucumber-cpp/autodetect.hpp>
#pragma GCC diagnostic pop
#include <vector>

using cucumber::ScenarioScope;
using namespace cddd::cqrs;

namespace {

boost::uuids::basic_random_generator<decltype(kerchow::picker)> generate_id{kerchow::picker};

class something_cool_happened {
public:
   explicit inline something_cool_happened(std::string what_) :
      what(std::move(what_))
   {
   }

   const std::string what;
};

class active_artifact : public cddd::cqrs::artifact {
public:
   active_artifact() :
      cddd::cqrs::artifact{generate_id()}
   {
      add_handler([this](const something_cool_happened &cool_thing) {
            cool_things_done.push_back(cool_thing.what);
         });
   }

   void do_something_cool(std::string what) {
      if (!what.empty()) {
         apply_change(something_cool_happened{std::move(what)});
      }
   }

   std::vector<std::string> cool_things_done;
};


class active_entity {
public:
   active_artifact entity;
   std::vector<std::string> things_done;
};


GIVEN("^I have an artifact that does cool things$") {
   ScenarioScope<active_entity> context;
}


GIVEN("^the artifact won the lottery$") {
   ScenarioScope<active_entity> context;
   const char *cool_thing = "wins the lottery";
   context->things_done.push_back(cool_thing);
   context->entity.apply_change(something_cool_happened{cool_thing});
}


WHEN("^it (.+)$") {
   REGEX_PARAM(std::string, cool_thing);
   ScenarioScope<active_entity> context;
   context->things_done.push_back(cool_thing);
   context->entity.do_something_cool(cool_thing);
}


THEN("^its events should reflect the cool things done$") {
   ScenarioScope<active_entity> context;

   ASSERT_EQ(context->things_done, context->entity.cool_things_done);
}

}
