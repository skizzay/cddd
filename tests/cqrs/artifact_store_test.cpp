#include "cqrs/artifact_store.h"
#include "cqrs/artifact.h"
#include <kerchow/kerchow.h>
#include <fakeit.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <gtest/gtest.h>
#include <deque>
#include <memory>


namespace {

using namespace fakeit;
using namespace cddd::cqrs;

boost::uuids::basic_random_generator<decltype(kerchow::picker)> gen_id{kerchow::picker};


auto nothing = [](auto &...) {};
auto dont_delete = [](auto *) {};


struct fake_event {};


template<class T>
class fake_factory {
public:
   explicit inline fake_factory(std::shared_ptr<T> mock_item) :
      item_to_return{mock_item}
   {
   }

   std::shared_ptr<T> operator()(const boost::uuids::uuid &) const {
      return item_to_return;
   }

private:
   std::shared_ptr<T> item_to_return;
};


typedef fake_factory<stream<domain_event_ptr>> fake_stream_factory;
typedef fake_factory<base_artifact> fake_artifact_factory;
typedef artifact_store<base_artifact, fake_stream_factory, fake_artifact_factory> store_type;


class artifact_store_test : public ::testing::Test {
public:
   virtual ~artifact_store_test() noexcept {
   }

   inline std::unique_ptr<store_type> create_target() {
      return std::make_unique<store_type>(
            std::shared_ptr<source<domain_event_ptr>>{&events_provider.get(), dont_delete},
            fake_stream_factory{std::shared_ptr<stream<domain_event_ptr>>{&events_stream.get(), dont_delete}},
            fake_artifact_factory{std::shared_ptr<base_artifact>{&entity.get(), dont_delete}}
         );
   }

   Mock<source<domain_event_ptr>> events_provider;
   Mock<stream<domain_event_ptr>> events_stream;
   Mock<base_artifact> entity;

   inline std::shared_ptr<base_artifact> entity_pointer() {
      return std::shared_ptr<base_artifact>{&entity.get(), dont_delete};
   }

private:
   std::shared_ptr<Mock<source<domain_event_ptr>>> mock_source;
   std::shared_ptr<Mock<stream<domain_event_ptr>>> mock_stream;
   std::shared_ptr<Mock<artifact>> mock_entity;
};


TEST_F(artifact_store_test, has_returns_false_when_object_id_is_null) {
   // Given
   auto target = create_target();

   // When
   bool actual = target->has(boost::uuids::nil_uuid());

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(artifact_store_test, has_returns_false_when_events_provider_does_not_have_artifact) {
   // Given
   auto target = create_target();
   auto id = gen_id();
   When(Method(events_provider, has).Using(id)).Return(false);

   // When
   bool actual = target->has(id);

   // Then
   ASSERT_FALSE(actual);
   Verify(Method(events_provider, has).Using(id)).Exactly(1_Time);
}


TEST_F(artifact_store_test, has_returns_true_when_events_provider_does_have_artifact) {
   // Given
   auto target = create_target();
   auto id = gen_id();
   When(Method(events_provider, has).Using(id)).Return(true);

   // When
   bool actual = target->has(id);

   // Then
   ASSERT_TRUE(actual);
   Verify(Method(events_provider, has).Using(id)).Exactly(1_Time);
}


TEST_F(artifact_store_test, put_throws_null_id_exception_when_object_id_is_null) {
   // Given
   auto target = create_target();
   When(Method(entity, id)).Return(boost::uuids::nil_uuid());

   // When
   ASSERT_THROW(target->put(entity_pointer()), cddd::utils::null_id_exception);
}


TEST_F(artifact_store_test, put_will_not_save_the_object_when_the_object_has_no_uncommitted_events) {
   // Given
   auto target = create_target();
   auto id = gen_id();
   When(Method(entity, has_uncommitted_events)).Return(false);
   When(Method(entity, id)).AlwaysReturn(id);

   // When
   target->put(entity_pointer());

   // Then
   VerifyNoOtherInvocations(events_provider);
}


TEST_F(artifact_store_test, put_will_save_the_object_when_the_object_has_uncommitted_events) {
   // Given
   using event_stream_ptr = std::shared_ptr<stream<domain_event_ptr>>;

   auto target = create_target();
   auto id = gen_id();
   auto es = std::shared_ptr<stream<domain_event_ptr>>{&events_stream.get(), dont_delete};
   When(Method(entity, id)).AlwaysReturn(id);
   When(Method(entity, has_uncommitted_events)).Return(true);
   When(Method(entity, clear_uncommitted_events)).Do([]{});
   When(Method(entity, uncommitted_events)).Do([]() { return domain_event_sequence{}; });
   When(Method(events_provider, has).Using(id)).Return(true);
   When(ConstOverloadedMethod(events_provider, get, event_stream_ptr(const boost::uuids::uuid &, size_t))).Return(es);
   When(Method(events_provider, put)).Do(nothing);
   When(Method(events_stream, save)).Do(nothing);

   // When
   target->put(entity_pointer());

   // Then
   Verify(Method(events_stream, save)).Exactly(1_Time);
}


TEST_F(artifact_store_test, put_will_create_a_stream_and_save_the_stream_when_the_source_does_not_have_a_stream) {
   // Given
   using event_stream_ptr = std::shared_ptr<stream<domain_event_ptr>>;

   auto target = create_target();
   auto id = gen_id();
   auto es = std::shared_ptr<stream<domain_event_ptr>>{&events_stream.get(), dont_delete};
   When(Method(entity, id)).AlwaysReturn(id);
   When(Method(entity, has_uncommitted_events)).Return(true);
   When(Method(entity, clear_uncommitted_events)).Do([]{});
   When(Method(entity, uncommitted_events)).Do([]() { return domain_event_sequence{}; });
   When(Method(events_provider, put)).Do(nothing);
   // save the the stream
   When(Method(events_stream, save)).Do(nothing);
   // the source does not have a stream
   When(Method(events_provider, has).Using(id)).Return(false);
   // will create a stream -> the factory does that part

   // When
   target->put(entity_pointer());

   // Then
   Verify(ConstOverloadedMethod(events_provider, get, event_stream_ptr(const boost::uuids::uuid &, size_t))).Exactly(0_Times);
   Verify(Method(events_stream, save)).Exactly(1_Time);
}


TEST_F(artifact_store_test, put_will_retrieve_a_stream_and_save_the_stream_when_the_source_does_have_a_stream) {
   // Given
   using event_stream_ptr = std::shared_ptr<stream<domain_event_ptr>>;

   auto target = create_target();
   auto id = gen_id();
   auto es = std::shared_ptr<stream<domain_event_ptr>>{&events_stream.get(), dont_delete};
   When(Method(entity, id)).AlwaysReturn(id);
   When(Method(entity, has_uncommitted_events)).Return(true);
   When(Method(entity, clear_uncommitted_events)).Do([]{});
   When(Method(entity, uncommitted_events)).Do([]() { return domain_event_sequence{}; });
   // will retrieve a stream
   When(ConstOverloadedMethod(events_provider, get, event_stream_ptr(const boost::uuids::uuid &, size_t))).Return(es);
   When(Method(events_provider, put)).Do(nothing);
   // save the the stream
   When(Method(events_stream, save)).Do(nothing);
   // the source does have a stream
   When(Method(events_provider, has).Using(id)).Return(true);

   // When
   target->put(entity_pointer());

   // Then
   Verify(ConstOverloadedMethod(events_provider, get, event_stream_ptr(const boost::uuids::uuid &, size_t))).Exactly(1_Time);
   Verify(Method(events_stream, save)).Exactly(1_Time);
}


TEST_F(artifact_store_test, get_will_throw_null_id_exception_when_object_id_is_null) {
   // Given
   auto target = create_target();

   // When
   ASSERT_THROW(target->get(boost::uuids::nil_uuid()), cddd::utils::null_id_exception);
}


TEST_F(artifact_store_test, get_with_version_will_throw_null_id_exception_when_object_id_is_null) {
   // Given
   auto target = create_target();
   size_t version = kerchow::picker.pick<size_t>(1);

   // When
   ASSERT_THROW(target->get(boost::uuids::nil_uuid(), version), cddd::utils::null_id_exception);
}


TEST_F(artifact_store_test, get_will_load_event_sequence_for_all_events) {
   // Given
   using event_stream_ptr = std::shared_ptr<stream<domain_event_ptr>>;

   auto target = create_target();
   boost::uuids::uuid id = gen_id();
   size_t version = std::numeric_limits<size_t>::max();
   size_t revision = 0;
   auto es = std::shared_ptr<stream<domain_event_ptr>>{&events_stream.get(), dont_delete};
   When(Method(entity, id)).AlwaysReturn(id);
   When(Method(entity, revision)).Return(revision);
   When(Method(entity, load_from_history)).Do(nothing);
   When(ConstOverloadedMethod(events_provider, get, event_stream_ptr(const boost::uuids::uuid &, size_t))).Return(es);
   When(ConstOverloadedMethod(events_stream, load, domain_event_sequence(size_t, size_t)).Using(revision + 1, version)).Do([](size_t, size_t) { return domain_event_sequence{}; });

   // When
   target->get(id);

   // Then
   Verify(ConstOverloadedMethod(events_stream, load, domain_event_sequence(size_t, size_t)).Using(revision + 1, version)).Exactly(1_Time);
}


TEST_F(artifact_store_test, get_with_version_will_load_event_sequence_for_requested_events) {
   // Given
   using event_stream_ptr = std::shared_ptr<stream<domain_event_ptr>>;

   auto target = create_target();
   boost::uuids::uuid id = gen_id();
   size_t version = kerchow::picker.pick<size_t>(20);
   size_t revision = kerchow::picker.pick<size_t>(1, 19);
   auto es = std::shared_ptr<stream<domain_event_ptr>>{&events_stream.get(), dont_delete};
   When(Method(entity, id)).AlwaysReturn(id);
   When(Method(entity, revision)).Return(revision);
   When(Method(entity, load_from_history)).Do(nothing);
   When(ConstOverloadedMethod(events_provider, get, event_stream_ptr(const boost::uuids::uuid &, size_t))).Return(es);
   When(ConstOverloadedMethod(events_stream, load, domain_event_sequence(size_t, size_t)).Using(revision + 1, version)).Do([](size_t, size_t) { return domain_event_sequence{}; });

   // When
   target->get(id, version);

   // Then
   Verify(ConstOverloadedMethod(events_stream, load, domain_event_sequence(size_t, size_t)).Using(revision + 1, version)).Exactly(1_Time);
}


TEST_F(artifact_store_test, get_with_revision_equal_to_requested_version_due_to_memento_loading_will_not_load_events) {
   // Given
   using event_stream_ptr = std::shared_ptr<stream<domain_event_ptr>>;

   auto target = create_target();
   boost::uuids::uuid id = gen_id();
   When(Method(entity, id)).AlwaysReturn(id);
   // with revision equal to requested version
   size_t version = kerchow::picker.pick<size_t>(1);
   // with revision equal to requested version
   size_t revision = version;
   // due to memento loading
   When(Method(entity, revision)).Return(revision);

   // When
   target->get(id, version);

   // Then
   // will not load events
   Verify(ConstOverloadedMethod(events_provider, get, event_stream_ptr(const boost::uuids::uuid &, size_t))).Exactly(0_Times);
   // will not load events
   Verify(ConstOverloadedMethod(events_stream, load, domain_event_sequence(size_t, size_t))).Exactly(0_Times);
}

}
