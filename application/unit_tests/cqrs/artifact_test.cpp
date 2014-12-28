#include "cqrs/artifact.h"
#include "cqrs/fakes/fake_dispatcher.h"
#include "cqrs/fakes/fake_event.h"
#include <gmock/gmock.h>


// The purpose of these tests is to ensure that our basic_artifact and artifact
// provides the base functionality that end users expect.  Conversely, the
// artifact tests that the inherited classes execute ensure that the behavior is
// consistent among all artifacts.
namespace {

using namespace cddd::cqrs;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrictMock;


class fake_event_collection {
public:
   class spy_type {
   public:
      typedef std::size_t size_type;

      MOCK_CONST_METHOD0(begin, void());
      MOCK_CONST_METHOD0(end, void());
      MOCK_CONST_METHOD0(empty, bool());
      MOCK_CONST_METHOD0(size, size_type());
      MOCK_METHOD0(clear, void());
      MOCK_METHOD1(push_back, void(domain_event_ptr));
   };

   typedef std::deque<domain_event_ptr>::const_iterator const_iterator;
   typedef std::size_t size_type;

   inline fake_event_collection(std::shared_ptr<spy_type> s) :
      spy(s)
   {
   }

   fake_event_collection() = delete;
   fake_event_collection(const fake_event_collection &) = default;

   inline const_iterator begin() const {
      spy->begin();
      return const_iterator();
   }

   inline const_iterator end() const {
      spy->end();
      return const_iterator();
   }

   inline bool empty() const {
      return spy->empty();
   }

   inline size_type size() const {
      return spy->size();
   }

   inline void clear() {
      spy->clear();
   }

   inline void push_back(domain_event_ptr e) {
      spy->push_back(e);
   }

   std::shared_ptr<spy_type> spy;
};


class artifact_spy : public basic_artifact<fake_dispatcher, fake_event_collection> {
public:
   typedef basic_artifact<fake_dispatcher, fake_event_collection> base_type;
   typedef fake_dispatcher::spy_type dispatcher_spy;
   typedef fake_event_collection::spy_type collection_spy;

   inline artifact_spy(std::shared_ptr<dispatcher_spy> ds, std::shared_ptr<collection_spy> cs) :
      base_type(0, std::make_shared<fake_dispatcher>(ds), fake_event_collection{cs})
   {
   }

   template<class Fun>
   inline void handle(Fun &&f) {
      add_handler(std::forward<Fun>(f));
   }
};


class artifact_test : public ::testing::Test {
public:
   inline auto create_target() {
      return artifact_spy{dispatcher_spy, collection_spy};
   }

   template<class T>
   inline void use_nice(std::shared_ptr<T> &pointer) const {
      pointer = std::make_shared<NiceMock<T>>();
   }

   template<class T>
   inline void use_strict(std::shared_ptr<T> &pointer) const {
      pointer = std::make_shared<StrictMock<T>>();
   }

   std::shared_ptr<fake_dispatcher::spy_type> dispatcher_spy = std::make_shared<fake_dispatcher::spy_type>();
   std::shared_ptr<fake_event_collection::spy_type> collection_spy = std::make_shared<fake_event_collection::spy_type>();
};


TEST_F(artifact_test, apply_change_does_invoke_dispatcher_to_dispatch_events) {
   // Given
   use_nice(collection_spy);
   use_strict(dispatcher_spy);
   auto target = create_target();
   fake_event event;
   EXPECT_CALL(*dispatcher_spy, dispatch_message(_)).
      Times(1);

   // When
   target.apply_change(std::move(event));

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(dispatcher_spy.get()));
}


TEST_F(artifact_test, apply_change_with_allocator_does_invoke_dispatcher_to_dispatch_events) {
   // Given
   use_nice(collection_spy);
   use_strict(dispatcher_spy);
   auto target = create_target();
   fake_event event;
   std::allocator<fake_event> allocator;
   EXPECT_CALL(*dispatcher_spy, dispatch_message(_)).
      Times(1);

   // When
   target.apply_change(allocator, std::move(event));

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(dispatcher_spy.get()));
}


TEST_F(artifact_test, has_uncommitted_events_returns_false_when_collection_is_empty) {
   // Given
   auto target = create_target();
   EXPECT_CALL(*collection_spy, empty())
         .WillOnce(Return(true));

   // When
   bool actual = target.has_uncommitted_events();

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(artifact_test, has_uncommitted_events_returns_true_when_collection_is_not_empty) {
   // Given
   auto target = create_target();
   EXPECT_CALL(*collection_spy, empty())
         .WillOnce(Return(false));

   // When
   bool actual = target.has_uncommitted_events();

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(artifact_test, add_handler_registers_event_handler) {
   // Given
   auto target = create_target();
   EXPECT_CALL(*dispatcher_spy, add_message_handler(_));

   // When
   target.handle([](fake_event){});

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(dispatcher_spy.get()));
}


TEST_F(artifact_test, revision_returns_0_without_applying_an_event) {
   // Given
   auto target = create_target();

   // When
   auto actual = target.revision();

   // Then
   ASSERT_EQ(0, actual);
}


TEST_F(artifact_test, has_uncommitted_events_returns_true_after_applying_an_event) {
   // Given
   use_nice(dispatcher_spy);
   use_strict(collection_spy);
   EXPECT_CALL(*collection_spy, push_back(_)).Times(1);
   EXPECT_CALL(*collection_spy, size()).WillOnce(Return(0));
   // Setting this expectation to return false because we're using a strict mock.  If things don't
   // go as expected, we'll fail elsewhere.
   EXPECT_CALL(*collection_spy, empty()).WillOnce(Return(false));
   auto target = create_target();
   fake_event e;
   target.apply_change(std::move(e));

   // When
   bool actual = target.has_uncommitted_events();

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(artifact_test, DISABLED_uncommitted_events_returns_a_sequence_with_a_single_event) {
   // Given
   use_nice(dispatcher_spy);
   use_nice(collection_spy);
   auto target = create_target();
   fake_event e;
   target.apply_change(std::move(e));

   // When
   auto actual = target.uncommitted_events() | sequencing::count();

   // Then
   ASSERT_EQ(1, actual);
}


TEST_F(artifact_test, has_uncommitted_events_returns_false_after_applying_an_event_and_clearing) {
   // Given
   use_nice(dispatcher_spy);
   use_strict(collection_spy);
   EXPECT_CALL(*collection_spy, push_back(_)).Times(1);
   EXPECT_CALL(*collection_spy, size()).WillOnce(Return(0));
   EXPECT_CALL(*collection_spy, clear()).Times(1);
   // Setting this expectation to return true because we're using a strict mock.  If things don't
   // go as expected, we'll fail elsewhere.
   EXPECT_CALL(*collection_spy, empty()).WillOnce(Return(true));
   auto target = create_target();
   fake_event e;
   target.apply_change(std::move(e));
   target.clear_uncommitted_events();

   // When
   bool actual = target.has_uncommitted_events();

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(artifact_test, DISABLED_uncommitted_events_returns_a_sequence_without_any_events_after_clearing) {
   // Given
   use_nice(dispatcher_spy);
   use_nice(collection_spy);
   auto target = create_target();
   fake_event e;
   target.apply_change(std::move(e));
   target.clear_uncommitted_events();

   // When
   auto actual = target.uncommitted_events() | sequencing::count();

   // Then
   ASSERT_EQ(0, actual);
}

}
