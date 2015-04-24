#include "cqrs/artifact_store.h"
#include "cqrs/fakes/fake_source.h"
#include "cqrs/fakes/fake_stream.h"
#include <boost/uuid/uuid_generators.hpp>
#include <gmock/gmock.h>
#include <deque>


namespace {

using namespace cddd::cqrs;
using ::testing::_;
using ::testing::A;
using ::testing::An;
using ::testing::AtLeast;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Ref;
using ::testing::Return;
using ::testing::StrictMock;

typedef fake_stream<std::shared_ptr<domain_event>> fake_event_stream;
typedef fake_stream_factory<std::shared_ptr<domain_event>> fake_event_stream_factory;
typedef fake_source<std::shared_ptr<domain_event>> fake_event_source;


class fake_entity_spy {
public:
   MOCK_CONST_METHOD0(id, void());
   MOCK_CONST_METHOD0(revision, void());
   MOCK_CONST_METHOD0(has_uncommitted_events, void());
   MOCK_CONST_METHOD0(uncommitted_events, void());
   MOCK_METHOD0(clear_uncommitted_events, void());
   MOCK_METHOD0(load_from_history, void());
};

boost::uuids::random_generator gen_id;


class fake_entity {
public:
   typedef sequencing::sequence<std::shared_ptr<domain_event>> event_sequence;

   inline const boost::uuids::uuid & id() const {
      spy->id();
      return id_value;
   }

   inline std::size_t revision() const {
      spy->revision();
      return entity_revision;
   }

   inline bool has_uncommitted_events() const {
      spy->has_uncommitted_events();
      return !pending.empty();
   }

   inline event_sequence uncommitted_events() const {
      spy->uncommitted_events();
      return sequencing::from(pending);
   }

   void clear_uncommitted_events() {
      spy->clear_uncommitted_events();
      pending.clear();
   }

   void load_from_history(event_sequence) {
      spy->load_from_history();
   }

   boost::uuids::uuid id_value = gen_id();
   std::size_t entity_revision = 0;
   std::deque<domain_event_ptr> pending;
   std::shared_ptr<fake_entity_spy> spy = std::make_shared<fake_entity_spy>();
};


class entity_factory_spy {
public:
   MOCK_CONST_METHOD1(create_fake_entity, std::shared_ptr<fake_entity>(const boost::uuids::uuid &));
};


class fake_entity_factory {
public:
   explicit inline fake_entity_factory(std::shared_ptr<entity_factory_spy> s=std::make_shared<entity_factory_spy>()) :
      spy(s)
   {
   }

   inline std::shared_ptr<fake_entity> operator()(const boost::uuids::uuid &id) const {
      return spy->create_fake_entity(id);
   }

   std::shared_ptr<entity_factory_spy> spy;
};


typedef artifact_store<fake_entity, fake_event_stream_factory, fake_entity_factory> store_type;


class artifact_store_test : public ::testing::Test {
   inline void set_default_behavior() {
      ON_CALL(*events_provider->spy, get(An<const boost::uuids::uuid &>(), An<std::size_t>()))
         .WillByDefault(Return(events_stream));
      ON_CALL(*stream_factory->spy, create_fake_stream(An<const boost::uuids::uuid &>()))
         .WillByDefault(Return(events_stream));
      ON_CALL(*entity_factory->spy, create_fake_entity(An<const boost::uuids::uuid &>()))
         .WillByDefault(Return(entity));
   }

public:
   inline artifact_store_test() :
      ::testing::Test(),
      events_stream(std::make_shared<fake_event_stream>()),
      stream_factory(std::make_shared<fake_event_stream_factory>()),
      events_provider(std::make_shared<fake_event_source>()),
      entity(std::make_shared<fake_entity>()),
      entity_factory(std::make_shared<fake_entity_factory>())
   {
      set_default_behavior();
   }

   inline std::unique_ptr<store_type> create_target() const {
      return std::make_unique<store_type>(events_provider, *stream_factory, *entity_factory);
   }

   template<class T>
   inline void use_nice(std::shared_ptr<T> &t) {
      t = std::make_shared<NiceMock<T>>();
      set_default_behavior();
   }

   template<class T>
   inline void use_strict(std::shared_ptr<T> &t) {
      t = std::make_shared<StrictMock<T>>();
   }

   std::shared_ptr<fake_event_stream> events_stream;
   std::shared_ptr<fake_event_stream_factory> stream_factory;
   std::shared_ptr<fake_event_source> events_provider;
   std::shared_ptr<fake_entity> entity;
   std::shared_ptr<fake_entity_factory> entity_factory;
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
   EXPECT_CALL(*events_provider->spy, has(An<const boost::uuids::uuid &>()))
      .WillOnce(Return(false));

   // When
   bool actual = target->has(gen_id());

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(artifact_store_test, has_returns_true_when_events_provider_does_have_artifact) {
   // Given
   auto target = create_target();
   EXPECT_CALL(*events_provider->spy, has(An<const boost::uuids::uuid &>()))
      .WillOnce(Return(true));

   // When
   bool actual = target->has(gen_id());

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(artifact_store_test, put_throws_null_id_exception_when_object_id_is_null) {
   // Given
   use_strict(entity->spy);
   auto target = create_target();
   entity->id_value = boost::uuids::nil_uuid();
   EXPECT_CALL(*entity->spy, id());

   // When
   ASSERT_THROW(target->put(entity), null_id_exception);
}


TEST_F(artifact_store_test, put_will_not_save_the_object_when_the_object_has_no_uncommitted_events) {
   // Given
   auto target = create_target();
   EXPECT_CALL(*entity->spy, has_uncommitted_events())
      .Times(1);
   EXPECT_CALL(*entity->spy, id())
      .Times(1);
   EXPECT_CALL(*entity->spy, uncommitted_events())
      .Times(0);
   EXPECT_CALL(*entity->spy, clear_uncommitted_events())
      .Times(0);
   EXPECT_CALL(*events_stream->spy, save())
      .Times(0);
   EXPECT_CALL(*events_provider->spy, put(_))
      .Times(0);

   // When
   target->put(entity);

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(entity->spy.get()));
   ASSERT_TRUE(Mock::VerifyAndClear(events_stream->spy.get()));
   ASSERT_TRUE(Mock::VerifyAndClear(events_provider->spy.get()));
}


TEST_F(artifact_store_test, put_will_save_the_object_when_the_object_has_uncommitted_events) {
   // Given
   auto target = create_target();
   entity->pending.push_back(nullptr);
   EXPECT_CALL(*entity->spy, has_uncommitted_events())
      .Times(1);
   EXPECT_CALL(*entity->spy, id())
      .Times(AtLeast(1));
   EXPECT_CALL(*entity->spy, uncommitted_events())
      .Times(1);
   EXPECT_CALL(*entity->spy, clear_uncommitted_events())
      .Times(1);
   EXPECT_CALL(*events_stream->spy, save())
      .Times(1);
   EXPECT_CALL(*stream_factory->spy, create_fake_stream(entity->id_value))
      .Times(1);
   EXPECT_CALL(*events_provider->spy, put(_))
      .Times(1);
   EXPECT_CALL(*events_provider->spy, has(entity->id_value))
      .Times(1);

   // When
   target->put(entity);

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(entity->spy.get()));
   ASSERT_TRUE(Mock::VerifyAndClear(events_stream->spy.get()));
   ASSERT_TRUE(Mock::VerifyAndClear(events_provider->spy.get()));
}


TEST_F(artifact_store_test, put_will_create_a_stream_and_save_the_stream_when_the_source_does_not_have_a_stream) {
   // Given
   use_strict(stream_factory->spy);
   use_nice(entity->spy);
   auto target = create_target();
   entity->pending.push_back(nullptr);
   EXPECT_CALL(*events_provider->spy, has(entity->id_value))
      .WillOnce(Return(false));
   EXPECT_CALL(*events_provider->spy, get(entity->id_value, An<std::size_t>()))
      .Times(0);
   EXPECT_CALL(*events_provider->spy, put(_))
      .Times(1);
   EXPECT_CALL(*stream_factory->spy, create_fake_stream(entity->id_value))
      .WillOnce(Return(events_stream));
   EXPECT_CALL(*events_stream->spy, save())
      .Times(1);

   // When
   target->put(entity);

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(stream_factory->spy.get()));
   ASSERT_TRUE(Mock::VerifyAndClear(events_provider->spy.get()));
}


TEST_F(artifact_store_test, put_will_retrieve_a_stream_and_save_the_stream_when_the_source_does_have_a_stream) {
   // Given
   use_strict(stream_factory->spy);
   use_nice(entity->spy);
   auto target = create_target();
   entity->pending.push_back(nullptr);
   EXPECT_CALL(*events_provider->spy, has(entity->id_value))
      .WillOnce(Return(true));
   EXPECT_CALL(*events_provider->spy, get(entity->id_value, An<std::size_t>()))
      .WillOnce(Return(events_stream));
   EXPECT_CALL(*events_provider->spy, put(_))
      .Times(1);
   EXPECT_CALL(*stream_factory->spy, create_fake_stream(entity->id_value))
      .Times(0);
   EXPECT_CALL(*events_stream->spy, save())
      .Times(1);

   // When
   target->put(entity);

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(stream_factory->spy.get()));
   ASSERT_TRUE(Mock::VerifyAndClear(events_provider->spy.get()));
}


TEST_F(artifact_store_test, get_will_throw_null_id_exception_when_object_id_is_null) {
   // Given
   auto target = create_target();

   // When
   ASSERT_THROW(target->get(boost::uuids::nil_uuid()), null_id_exception);
}


TEST_F(artifact_store_test, get_with_version_will_throw_null_id_exception_when_object_id_is_null) {
   // Given
   auto target = create_target();
   std::size_t version = static_cast<std::size_t>(std::rand());

   // When
   ASSERT_THROW(target->get(boost::uuids::nil_uuid(), version), null_id_exception);
}


TEST_F(artifact_store_test, get_will_load_event_sequence_for_all_events) {
   // Given
   use_nice(entity_factory->spy);
   use_nice(entity->spy);
   auto target = create_target();
   boost::uuids::uuid id = gen_id();
   std::size_t version = static_cast<std::size_t>(std::rand());
   EXPECT_CALL(*events_provider->spy, get(id, std::numeric_limits<std::size_t>::max()))
      .WillOnce(Return(events_stream));
   EXPECT_CALL(*events_stream->spy, load(1, version))
      .Times(1);

   // When
   target->get(id, version);

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(events_provider->spy.get()));
   ASSERT_TRUE(Mock::VerifyAndClear(events_stream->spy.get()));
}


TEST_F(artifact_store_test, get_with_version_will_load_event_sequence_for_requested_events) {
   // Given
   use_nice(entity_factory->spy);
   use_nice(entity->spy);
   auto target = create_target();
   boost::uuids::uuid id = gen_id();
   EXPECT_CALL(*events_provider->spy, get(id, std::numeric_limits<std::size_t>::max()))
      .WillOnce(Return(events_stream));
   EXPECT_CALL(*events_stream->spy, load(1, std::numeric_limits<std::size_t>::max()))
      .Times(1);

   // When
   target->get(id);

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(events_provider->spy.get()));
   ASSERT_TRUE(Mock::VerifyAndClear(events_stream->spy.get()));
}


TEST_F(artifact_store_test, get_invoke_the_artifact_factory_to_create_artifact_instance) {
   // Given
   use_nice(events_provider->spy);
   use_nice(events_stream->spy);
   use_nice(entity->spy);
   auto target = create_target();
   boost::uuids::uuid id = gen_id();
   EXPECT_CALL(*entity_factory->spy, create_fake_entity(id))
      .WillOnce(Return(entity));

   // When
   target->get(id);

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(entity_factory->spy.get()));
}


TEST_F(artifact_store_test, get_loads_the_history_onto_the_entity_after_instantiation) {
   // Given
   use_nice(entity_factory->spy);
   use_nice(events_stream->spy);
   use_nice(events_provider->spy);
   std::size_t version = static_cast<std::size_t>(std::rand());
   auto target = create_target();
   boost::uuids::uuid id = gen_id();
   EXPECT_CALL(*entity->spy, load_from_history())
      .Times(1);
   EXPECT_CALL(*entity->spy, revision())
      .Times(1);

   // When
   target->get(id, version);

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(entity->spy.get()));
}

}
