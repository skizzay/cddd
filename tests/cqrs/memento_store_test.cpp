#include "cqrs/artifact.h"
#include "cqrs/memento_store.h"
#include <boost/uuid/uuid_generators.hpp>
#include <gtest/gtest.h>
#include <fakeit.hpp>
#include <kerchow/kerchow.h>

namespace {

using namespace fakeit;
using namespace cddd::cqrs;
using kerchow::picker;

boost::uuids::basic_random_generator<decltype(kerchow::picker)> gen_id{kerchow::picker};

class artifact_without_snapshots : public artifact {
public:
   using artifact::basic_artifact;

   inline artifact_without_snapshots() :
      artifact{gen_id()}
   {
   }
};


class artifact_with_snapshots : public artifact {
public:
   using artifact::basic_artifact;
   typedef int memento_type;

   inline artifact_with_snapshots() :
      artifact{gen_id()}
   {
   }
};


TEST(memento_store_test, given_artifact_without_snapshots_when_applying_memento_to_artifact_then_should_do_nothing) {
   // This is kind of a bullshit test.  We basically are just checking that we can compile.
   // Given
   artifact_without_snapshots object;
   memento_store<artifact_without_snapshots> target;

   // When
   memento_store<artifact_without_snapshots>::apply_memento_to_object(object, picker.pick<std::size_t>(1), target);

   // Then (do nothing aka compilation == success)
}


TEST(memento_store_test, given_artifact_with_snapshots_when_applying_memento_to_artifact_then_should_invoke_application_method) {
   // Given
   artifact_with_snapshots object;
   Mock<memento_store<artifact_with_snapshots>> target;
   When(Method(target, apply_memento_to_object)).Do([](artifact_with_snapshots &, std::size_t){});

   // When
   memento_store<artifact_with_snapshots>::apply_memento_to_object(object, picker.pick<std::size_t>(1), target.get());

   // Then
   Verify(Method(target, apply_memento_to_object)).Exactly(1_Time);
}

}
