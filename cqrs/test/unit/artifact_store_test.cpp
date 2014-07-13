#include "cddd/cqrs/artifact_store.h"
#include "cddd/cqrs/test/fakes/fake_event_source.h"
#include "cddd/cqrs/test/fakes/fake_event_stream.h"
#include <deque>
#include <gmock/gmock.h>


namespace {

using namespace cddd::cqrs;
using ::testing::_;
using ::testing::An;
using ::testing::AtLeast;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Ref;
using ::testing::Return;


class fake_entity_spy {
public:
   MOCK_CONST_METHOD0(id, void());
   MOCK_CONST_METHOD0(has_uncommitted_events, void());
   MOCK_CONST_METHOD0(uncommitted_events, void());
   MOCK_METHOD0(clear_uncommitted_events, void());
   MOCK_METHOD0(load_from_history, void());
};


class fake_entity {
public:
   inline object_id id() const {
      spy->id();
      return id_value;
   }

   inline bool has_uncommitted_events() const {
      spy->has_uncommitted_events();
      return !pending.empty();
   }

   inline event_sequence uncommitted_events() const {
      spy->uncommitted_events();
      return from(pending);
   }

   void clear_uncommitted_events() {
      spy->clear_uncommitted_events();
      pending.clear();
   }

   void load_from_history(event_sequence) {
      spy->load_from_history();
   }

   object_id id_value;
   std::deque<event_ptr> pending;
   std::shared_ptr<NiceMock<fake_entity_spy>> spy = std::make_shared<NiceMock<fake_entity_spy>>();
};

typedef NiceMock<fake_entity> entity_type;

typedef fake_event_stream event_stream_type;


class stream_factory_spy {
public:
   MOCK_CONST_METHOD1(create_fake_stream, std::shared_ptr<event_stream_type>(object_id));
};


class fake_stream_factory {
public:
   explicit inline fake_stream_factory(std::shared_ptr<NiceMock<stream_factory_spy>> s) :
      spy(s)
   {
   }

   inline std::shared_ptr<event_stream_type> operator()(object_id id) const {
      return spy->create_fake_stream(id);
   }

   std::shared_ptr<NiceMock<stream_factory_spy>> spy;
};


class entity_factory_spy {
public:
   MOCK_CONST_METHOD1(create_fake_entity, std::shared_ptr<entity_type>(object_id));
};


class fake_entity_factory {
public:
   explicit inline fake_entity_factory(std::shared_ptr<NiceMock<entity_factory_spy>> s) :
      spy(s)
   {
   }

   inline std::shared_ptr<entity_type> operator()(object_id id) const {
      return spy->create_fake_entity(id);
   }

   std::shared_ptr<NiceMock<entity_factory_spy>> spy;
};


typedef NiceMock<fake_event_source> event_source_type;
typedef artifact_store<entity_type, event_source_type,
                       fake_stream_factory, fake_entity_factory> store_type;


class artifact_store_test : public ::testing::Test {
public:
   inline artifact_store_test() :
      ::testing::Test(),
      events_stream(std::make_shared<event_stream_type>()),
      events_provider(std::make_unique<event_source_type>()),
      events_provider_spy(events_provider->spy),
      stream_factory(std::make_shared<NiceMock<stream_factory_spy>>()),
      entity(std::make_shared<entity_type>()),
      entity_factory(std::make_shared<NiceMock<entity_factory_spy>>())
   {
      ON_CALL(*events_provider->spy, get(An<object_id>()))
         .WillByDefault(Return(events_stream));
      ON_CALL(*stream_factory, create_fake_stream(An<object_id>()))
         .WillByDefault(Return(events_stream));
      ON_CALL(*entity_factory, create_fake_entity(An<object_id>()))
         .WillByDefault(Return(entity));
   }

   std::unique_ptr<store_type> create() {
      return std::make_unique<store_type>(std::move(events_provider), fake_stream_factory{stream_factory}, fake_entity_factory{entity_factory});
   }

   std::shared_ptr<event_stream_type> events_stream;
   std::unique_ptr<event_source_type> events_provider;
   std::shared_ptr<event_source_spy> events_provider_spy;
   std::shared_ptr<NiceMock<stream_factory_spy>> stream_factory;
   std::shared_ptr<entity_type> entity;
   std::shared_ptr<NiceMock<entity_factory_spy>> entity_factory;
};


TEST_F(artifact_store_test, has_returns_false_when_object_id_is_null) {
   // Given
   auto target = create();

   // When
   bool actual = target->has(object_id{});

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(artifact_store_test, has_returns_false_when_events_provider_does_not_have_artifact) {
   // Given
   auto target = create();
   EXPECT_CALL(*events_provider_spy, has(An<object_id>()))
      .WillOnce(Return(false));

   // When
   bool actual = target->has(object_id::create(1));

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(artifact_store_test, has_returns_true_when_events_provider_does_have_artifact) {
   // Given
   auto target = create();
   EXPECT_CALL(*events_provider_spy, has(An<object_id>()))
      .WillOnce(Return(true));

   // When
   bool actual = target->has(object_id::create(1));

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(artifact_store_test, put_throws_null_pointer_exception_when_object_is_null) {
   // Given
   auto target = create();

   // When
   ASSERT_THROW(target->put(nullptr), null_pointer_exception);
}


TEST_F(artifact_store_test, put_throws_null_id_exception_when_object_id_is_null) {
   // Given
   auto target = create();

   // When
   ASSERT_THROW(target->put(entity), null_id_exception);
}


TEST_F(artifact_store_test, put_will_not_save_the_object_when_the_object_has_no_uncommitted_events) {
   // Given
   auto target = create();
   entity->id_value = object_id::create(1);
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
   EXPECT_CALL(*events_provider_spy, put(_))
      .Times(0);

   // When
   target->put(entity);

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(entity->spy.get()));
   ASSERT_TRUE(Mock::VerifyAndClear(events_stream->spy.get()));
   ASSERT_TRUE(Mock::VerifyAndClear(events_provider_spy.get()));
}


TEST_F(artifact_store_test, put_will_save_the_object_when_the_object_has_uncommitted_events) {
   // Given
   auto target = create();
   entity->id_value = object_id::create(1);
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
   EXPECT_CALL(*events_provider_spy, put(_))
      .Times(1);

   // When
   target->put(entity);

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(entity->spy.get()));
   ASSERT_TRUE(Mock::VerifyAndClear(events_stream->spy.get()));
   ASSERT_TRUE(Mock::VerifyAndClear(events_provider_spy.get()));
}


TEST_F(artifact_store_test, put_will_create_a_stream_when_the_source_does_not_have_a_stream) {
   // Given
   auto target = create();
   entity->id_value = object_id::create(1);
   entity->pending.push_back(nullptr);
   EXPECT_CALL(*events_provider_spy, has(entity->id_value))
      .WillOnce(Return(false));
   EXPECT_CALL(*events_provider_spy, get(entity->id_value))
      .Times(0);
   EXPECT_CALL(*stream_factory, create_fake_stream(entity->id_value))
      .WillOnce(Return(events_stream));

   // When
   target->put(entity);

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(stream_factory.get()));
   ASSERT_TRUE(Mock::VerifyAndClear(events_provider_spy.get()));
}


TEST_F(artifact_store_test, put_will_retrieve_a_stream_when_the_source_does_have_a_stream) {
   // Given
   auto target = create();
   entity->id_value = object_id::create(1);
   entity->pending.push_back(nullptr);
   EXPECT_CALL(*events_provider_spy, has(entity->id_value))
      .WillOnce(Return(true));
   EXPECT_CALL(*events_provider_spy, get(entity->id_value))
      .WillOnce(Return(events_stream));
   EXPECT_CALL(*stream_factory, create_fake_stream(entity->id_value))
      .Times(0);

   // When
   target->put(entity);

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(stream_factory.get()));
   ASSERT_TRUE(Mock::VerifyAndClear(events_provider_spy.get()));
}


TEST_F(artifact_store_test, get_will_throw_null_id_exception_when_object_id_is_null) {
   // Given
   auto target = create();
   object_id null_id;

   // When
   ASSERT_THROW(target->get(null_id), null_id_exception);
}


TEST_F(artifact_store_test, get_with_version_will_throw_null_id_exception_when_object_id_is_null) {
   // Given
   auto target = create();
   object_id null_id;
   std::size_t version = static_cast<std::size_t>(std::rand());

   // When
   ASSERT_THROW(target->get(null_id, version), null_id_exception);
}


TEST_F(artifact_store_test, get_will_load_event_sequence_for_all_events) {
   // Given
   auto target = create();
   object_id id = object_id::create(1);
   std::size_t version = static_cast<std::size_t>(std::rand());
   EXPECT_CALL(*events_provider_spy, get(id))
      .WillOnce(Return(events_stream));
   EXPECT_CALL(*events_stream->spy, load(0, version))
      .Times(1);

   // When
   target->get(id, version);

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(events_provider_spy.get()));
   ASSERT_TRUE(Mock::VerifyAndClear(events_stream->spy.get()));
}


TEST_F(artifact_store_test, get_with_version_will_load_event_sequence_for_requested_events) {
   // Given
   auto target = create();
   object_id id = object_id::create(1);
   EXPECT_CALL(*events_provider_spy, get(id))
      .WillOnce(Return(events_stream));
   EXPECT_CALL(*events_stream->spy, load(0, std::numeric_limits<std::size_t>::max()))
      .Times(1);

   // When
   target->get(id);

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(events_provider_spy.get()));
   ASSERT_TRUE(Mock::VerifyAndClear(events_stream->spy.get()));
}


TEST_F(artifact_store_test, get_invoke_the_artifact_factory_to_create_artifact_instance) {
   // Given
   auto target = create();
   object_id id = object_id::create(1);
   EXPECT_CALL(*entity_factory, create_fake_entity(id))
      .WillOnce(Return(entity));

   // When
   target->get(id);

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(entity_factory.get()));
}


TEST_F(artifact_store_test, get_loads_the_history_onto_the_entity_after_instantiation) {
   // Given
   auto target = create();
   object_id id = object_id::create(1);
   EXPECT_CALL(*entity->spy, load_from_history())
      .Times(1);

   // When
   target->get(id);

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(entity->spy.get()));
}

}
