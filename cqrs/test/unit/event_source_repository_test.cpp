#include "cddd/cqrs/event_source_repository.h"
#include "cddd/cqrs/test/fakes/fake_event_store.h"
#include "cddd/cqrs/test/fakes/fake_event_stream.h"
#include "cddd/cqrs/test/fakes/fake_object_id_generator.h"
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


class fake_entity {
public:
   inline fake_entity() :
      pending()
   {
   }

   MOCK_CONST_METHOD0(id, object_id());

   inline bool has_uncommitted_events() const {
      return !pending.empty();
   }

   inline event_sequence uncommitted_events() const {
      return event_sequence::from(pending);
   }

   void clear_uncommitted_events() {
      pending.clear();
   }

   MOCK_METHOD1(load_from_history, void(event_stream &));

   std::deque<event_ptr> pending;
};


class fake_entity_factory {
public:
   MOCK_METHOD1(create_fake_entity, std::shared_ptr<fake_entity>(object_id));

   inline std::shared_ptr<fake_entity> operator()(object_id id) {
      return create_fake_entity(id);
   }
};


class event_source_repository_tests : public ::testing::Test {
public:
   typedef event_source_repository<fake_entity> repository_type;
   typedef NiceMock<fake_event_store> event_store_type;
   typedef NiceMock<fake_event_stream> event_stream_type;

   inline event_source_repository_tests() :
      ::testing::Test(),
      store(std::make_shared<event_store_type>()),
      stream(std::make_shared<event_stream_type>()),
      generator(),
      factory()
   {
      ON_CALL(*store, open_stream(_))
         .WillByDefault(Return(stream));
   }

   std::unique_ptr<repository_type> create() {
      return std::make_unique<repository_type>(store, std::ref(generator), std::ref(factory));
   }

   std::shared_ptr<event_store_type> store;
   std::shared_ptr<event_stream_type> stream;
   NiceMock<fake_object_id_generator> generator;
   NiceMock<fake_entity_factory> factory;
};


TEST_F(event_source_repository_tests, has_returns_false_when_stream_has_no_committed_events) {
   // Given
   auto target = create();
   EXPECT_CALL(*stream, has_committed_events())
      .WillOnce(Return(false));

   // When
   bool actual = target->has(object_id());

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(event_source_repository_tests, has_returns_true_when_stream_has_committed_events) {
   // Given
   auto target = create();
   EXPECT_CALL(*stream, has_committed_events())
      .WillOnce(Return(true));

   // When
   bool actual = target->has(object_id());

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(event_source_repository_tests, load_uses_the_factory_to_create_object) {
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


TEST_F(event_source_repository_tests, load_has_the_loaded_object_to_load_from_history) {
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

}
