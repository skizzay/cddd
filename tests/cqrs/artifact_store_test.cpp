#include "cqrs/artifact_store.h"
#include "cqrs/artifact.h"
#include "cqrs/commit.h"
#include "cqrs/fakes/fake_event.h"
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


class test_artifact final : public artifact {
public:
   using basic_artifact::set_version;

   explicit inline test_artifact() :
      artifact{gen_id()}
   {
      add_handler([](const fake_event &) {});
   }
};


class base_domain_event_stream : public stream<base_domain_event_stream> {
public:
   typedef std::shared_ptr<domain_event> value_type;

   virtual ~base_domain_event_stream() noexcept = default;
   virtual sequencing::sequence<value_type> load_revisions(std::size_t, std::size_t) const = 0;
   virtual void save_sequence(sequencing::sequence<value_type> &&) = 0;
   virtual commit persist_changes() = 0;
};


class base_domain_event_stream_store : public domain_event_stream_store<base_domain_event_stream_store> {
public:
   virtual ~base_domain_event_stream_store() noexcept = default;
   virtual bool has_stream_for(const boost::uuids::uuid &) const = 0;
   virtual std::shared_ptr<base_domain_event_stream> get_stream_for(const boost::uuids::uuid &) = 0;
   virtual std::shared_ptr<base_domain_event_stream> create_stream_for(const boost::uuids::uuid &) = 0;
};


class test_artifact_factory {
public:
   struct interface {
      virtual ~interface() noexcept = default;
      virtual std::shared_ptr<test_artifact> create_test_artifact(const boost::uuids::uuid &id) = 0;
   };

   // For testing, we're allowing implicit construction to make life easier.
   inline test_artifact_factory(interface &pimpl_) :
      pimpl{pimpl_}
   {
   }

   inline auto operator()(const boost::uuids::uuid &id) {
      return pimpl.create_test_artifact(id);
   }

private:
   interface &pimpl;
};


typedef artifact_store<test_artifact, base_domain_event_stream_store, test_artifact_factory> store_type;


class artifact_store_test : public ::testing::Test {
public:
   virtual ~artifact_store_test() noexcept {
   }

   inline std::unique_ptr<store_type> create_target() {
      return std::make_unique<store_type>(events_provider.get(), artifact_factory.get());
   }

   inline commit create_commit() {
      return commit{gen_id(),
                    gen_id(),
                    kerchow::picker.pick<std::size_t>(1),
                    kerchow::picker.pick<std::size_t>(1),
                    boost::posix_time::microsec_clock::universal_time()};
   }

   Mock<base_domain_event_stream_store> events_provider;
   Mock<test_artifact_factory::interface> artifact_factory;
   Mock<base_domain_event_stream> events_stream;
   test_artifact entity;
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
   When(Method(events_provider, has_stream_for).Using(id)).Return(false);

   // When
   bool actual = target->has(id);

   // Then
   ASSERT_FALSE(actual);
   Verify(Method(events_provider, has_stream_for).Using(id)).Exactly(1_Time);
}


TEST_F(artifact_store_test, has_returns_true_when_events_provider_does_have_artifact) {
   // Given
   auto target = create_target();
   auto id = gen_id();
   When(Method(events_provider, has_stream_for).Using(id)).Return(true);

   // When
   bool actual = target->has(id);

   // Then
   ASSERT_TRUE(actual);
   Verify(Method(events_provider, has_stream_for).Using(id)).Exactly(1_Time);
}


TEST_F(artifact_store_test, put_will_not_save_the_object_when_the_object_has_no_uncommitted_events) {
   // Given
   auto target = create_target();

   // When
   target->put(entity);

   // Then
   VerifyNoOtherInvocations(events_provider);
}


TEST_F(artifact_store_test, put_will_return_noncommit_when_the_object_has_no_uncommitted_events) {
   // Given
   auto target = create_target();

   // When
   auto actual = target->put(entity);

   // Then
   ASSERT_TRUE(actual.is_noncommit());
}


TEST_F(artifact_store_test, put_will_save_the_object_when_the_object_has_uncommitted_events) {
   // Given
   auto target = create_target();
   auto es = std::shared_ptr<base_domain_event_stream>{&events_stream.get(), dont_delete};
   When(Method(events_provider, has_stream_for).Using(entity.id())).Return(true);
   When(Method(events_provider, get_stream_for).Using(entity.id())).Return(es);
   When(Method(events_stream, save_sequence)).Do(nothing);
   When(Method(events_stream, persist_changes)).Return(create_commit());
   entity.apply_change(fake_event{});

   // When
   target->put(entity);

   // Then
   Verify(Method(events_stream, save_sequence)).Exactly(1_Time);
   Verify(Method(events_stream, persist_changes)).Exactly(1_Time);
}


TEST_F(artifact_store_test, put_will_return_valid_commit_when_the_object_has_uncommitted_events) {
   // Given
   auto target = create_target();
   auto es = std::shared_ptr<base_domain_event_stream>{&events_stream.get(), dont_delete};
   When(Method(events_provider, has_stream_for).Using(entity.id())).Return(true);
   When(Method(events_provider, get_stream_for).Using(entity.id())).Return(es);
   When(Method(events_stream, save_sequence)).Do(nothing);
   When(Method(events_stream, persist_changes)).Return(create_commit());
   entity.apply_change(fake_event{});

   // When
   auto actual = target->put(entity);

   // Then
   ASSERT_FALSE(actual.is_noncommit());
}


TEST_F(artifact_store_test, get_will_throw_null_id_exception_when_object_id_is_null) {
   // Given
   auto target = create_target();
   size_t version = kerchow::picker.pick<size_t>();

   // When
   ASSERT_THROW(target->get(boost::uuids::nil_uuid(), version), cddd::utils::null_id_exception);
}


TEST_F(artifact_store_test, get_will_load_event_sequence_for_all_events) {
   // Given
   typedef std::shared_ptr<domain_event> value_type;

   auto target = create_target();
   boost::uuids::uuid id = entity.id();
   size_t version = std::numeric_limits<size_t>::max();
   size_t revision = 0;
   auto es = std::shared_ptr<base_domain_event_stream>{&events_stream.get(), dont_delete};
   auto entity_pointer = std::shared_ptr<test_artifact>{&entity, dont_delete};
   When(Method(artifact_factory, create_test_artifact).Using(id)).Return(entity_pointer);
   When(Method(events_provider, has_stream_for).Using(id)).Return(false);
   When(Method(events_provider, create_stream_for).Using(id)).Return(es);
   When(Method(events_stream, load_revisions).Using(revision + 1, version)).Do([](size_t, size_t) { return sequencing::sequence<value_type>{}; });

   // When
   target->get(id, version);

   // Then
   Verify(Method(events_stream, load_revisions).Using(revision + 1, version)).Exactly(1_Time);
}


TEST_F(artifact_store_test, get_with_revision_equal_to_requested_version_due_to_memento_loading_will_not_load_events) {
   // Given
   auto target = create_target();
   boost::uuids::uuid id = entity.id();
   size_t version = kerchow::picker.pick<size_t>(1);
   auto entity_pointer = std::shared_ptr<test_artifact>{&entity, dont_delete};
   When(Method(artifact_factory, create_test_artifact).Using(id)).Return(entity_pointer);
   entity.set_version(version);

   // When
   target->get(id, version);

   // Then
   Verify(Method(events_stream, load_revisions)).Exactly(0_Times);
   Verify(Method(events_provider, has_stream_for)).Exactly(0_Times);
}

}
