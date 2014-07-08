#include "cddd/cqrs/artifact_store.h"
#include "cddd/cqrs/test/fakes/fake_event_source.h"
#include "cddd/cqrs/test/fakes/fake_event_stream.h"
#include <deque>
#include <gmock/gmock.h>


namespace {

using namespace cddd::cqrs;
using ::testing::_;
using ::testing::An;
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
   std::shared_ptr<fake_entity_spy> spy = std::make_shared<fake_entity_spy>();
};

typedef NiceMock<fake_entity> entity_type;

typedef NiceMock<fake_event_stream> event_stream_type;


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


class artifact_store_tests : public ::testing::Test {
public:
   inline artifact_store_tests() :
      ::testing::Test(),
      events_stream(std::make_shared<event_stream_type>()),
      events_provider(std::make_unique<event_source_type>()),
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
   std::shared_ptr<NiceMock<stream_factory_spy>> stream_factory;
   std::shared_ptr<entity_type> entity;
   std::shared_ptr<NiceMock<entity_factory_spy>> entity_factory;
};


#if 0
TEST_F(artifact_store_tests, has_returns_false_when_stream_has_no_committed_events) {
   // Given
   auto target = create();
   EXPECT_CALL(*stream, has_committed_events())
      .WillOnce(Return(false));

   // When
   bool actual = target->has(object_id());

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(artifact_store_tests, has_returns_true_when_stream_has_committed_events) {
   // Given
   auto target = create();
   EXPECT_CALL(*stream, has_committed_events())
      .WillOnce(Return(true));

   // When
   bool actual = target->has(object_id());

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(artifact_store_tests, load_uses_the_factory_to_create_object) {
   // Given
   auto target = create();
   std::shared_ptr<NiceMock<fake_entity>> entity = std::make_shared<NiceMock<fake_entity>>();
   EXPECT_CALL(factory, create_fake_entity(_))
      .WillOnce(Return(entity));

   // When
   target->load(object_id());

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(&factory));
}


TEST_F(artifact_store_tests, load_has_the_loaded_object_to_load_from_history) {
   // Given
   auto target = create();
   auto entity = std::make_shared<NiceMock<fake_entity>>();
   ON_CALL(factory, create_fake_entity(An<object_id>()))
      .WillByDefault(Return(entity));
   EXPECT_CALL(*entity, load_from_history(_));

   // When
   target->load(object_id());

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(&factory));
}
#endif

}
