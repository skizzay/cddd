#include "cddd/cqrs/event_source_repository.h"
#include "cddd/cqrs/test/fakes/fake_event_store.h"
#include "cddd/cqrs/test/fakes/fake_event_stream.h"
#include "cddd/cqrs/test/fakes/fake_object_id_generator.h"
#include <deque>
#include <gmock/gmock.h>


namespace {

using namespace cddd::cqrs;
using ::testing::_;
using ::testing::NiceMock;
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
   fake_object_id_generator generator;
   fake_entity_factory factory;
};


TEST_F(event_source_repository_tests, has_returns_false_when_stream_has_no_committed_events) {
   // Given
   auto target = create();

   // When
   bool actual = target->has(object_id());

   // Then
   ASSERT_FALSE(actual);
}

}
