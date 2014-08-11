#include "cqrs/artifact.h"
#include "cqrs/test/fakes/fake_event.h"
#include <gmock/gmock.h>


// The purpose of these tests is to ensure that our basic_artifact and artifact
// provides the base functionality that end users expect.  Conversely, the
// artifact tests that the inherited classes execute ensure that the behavior is
// consistent among all artifacts.
namespace {

using namespace cddd::cqrs;
using ::testing::_;
using ::testing::Mock;
using ::testing::NiceMock;


typedef std::function<void(const event &)> event_handler;


class dispatcher_spy {
public:
   MOCK_METHOD2(add_handler, void(std::type_index, event_handler));
   MOCK_METHOD1(dispatch, void(const event &));
};


class fake_dispatcher {
public:
   typedef std::shared_ptr<NiceMock<dispatcher_spy>> spy_ptr;

   inline fake_dispatcher(spy_ptr s) :
      spy(s)
   {
   }

   fake_dispatcher() = delete;
   fake_dispatcher(const fake_dispatcher &) = default;

   template<class Evt, class Fun>
   inline void add_handler(Fun f) {
      event_handler closure = [f=std::move(f)](const event &e) {
         f(static_cast<const cddd::cqrs::details_::event_wrapper<Evt>&>(e).evt);
      };
      spy->add_handler(typeid(Evt), std::move(closure));
   }

   inline void dispatch(const event &e) {
      spy->dispatch(e);
   }

   spy_ptr spy;
};


class event_collection_spy {
public:
   typedef std::size_t size_type;

   MOCK_CONST_METHOD0(begin, void());
   MOCK_CONST_METHOD0(end, void());
   MOCK_CONST_METHOD0(empty, void());
   MOCK_CONST_METHOD0(size, size_type());
   MOCK_METHOD0(clear, void());
   MOCK_METHOD1(push_back, void(event_ptr));
};


class fake_event_collection {
public:
   typedef std::deque<event_ptr>::const_iterator const_iterator;
   typedef std::shared_ptr<NiceMock<event_collection_spy>> spy_ptr;
   typedef std::shared_ptr<std::deque<event_ptr>> impl_ptr;
   typedef std::size_t size_type;

   inline fake_event_collection(spy_ptr s, impl_ptr i) :
      spy(s),
      impl(i)
   {
   }

   fake_event_collection() = delete;
   fake_event_collection(const fake_event_collection &) = default;

   inline const_iterator begin() const {
      spy->begin();
      return impl->begin();
   }

   inline const_iterator end() const {
      spy->end();
      return impl->end();
   }

   inline bool empty() const {
      spy->empty();
      return impl->empty();
   }

   inline size_type size() const {
      spy->size();
      return impl->size();
   }

   inline void clear() {
      spy->clear();
      impl->clear();
   }

   inline void push_back(event_ptr e) {
      spy->push_back(e);
      impl->push_back(e);
   }

   spy_ptr spy;
   impl_ptr impl;
};


class artifact_spy : public basic_artifact<fake_dispatcher, fake_event_collection> {
public:
   typedef basic_artifact<fake_dispatcher, fake_event_collection> base_type;
   typedef fake_event_collection::spy_ptr ecs_ptr;
   typedef fake_event_collection::impl_ptr eci_ptr;
   typedef fake_dispatcher::spy_ptr ds_ptr;

   inline artifact_spy(ecs_ptr ecs_, eci_ptr eci_, ds_ptr ds_) :
      base_type(fake_dispatcher(ds_),
                fake_event_collection(ecs_, eci_)),
      ecs(ecs_),
      eci(eci_),
      ds(ds_)
   {
   }

   template<class Evt, class Fun>
   inline void handle(Fun f) {
      add_handler<Evt>(std::move(f));
   }

   ecs_ptr ecs;
   eci_ptr eci;
   ds_ptr ds;
};


inline artifact_spy create_target() {
   fake_event_collection::spy_ptr ecs = std::make_shared<NiceMock<event_collection_spy>>();
   fake_event_collection::impl_ptr eci = std::make_shared<std::deque<event_ptr>>();
   fake_dispatcher::spy_ptr ds = std::make_shared<NiceMock<dispatcher_spy>>();

   return artifact_spy(ecs, eci, ds);
}


class artifact_test : public ::testing::Test {
public:
   inline artifact_test() :
      ::testing::Test(),
      target(create_target())
   {
   }

   artifact_spy target;
};


TEST_F(artifact_test, apply_change_invokes_dispatcher_to_dispatch_events) {
   // Given
   fake_event event;
   EXPECT_CALL(*target.ds, dispatch(_));

   // When
   target.apply_change(std::move(event));

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(target.ds.get()));
}


TEST_F(artifact_test, apply_change_with_allocator_invokes_dispatcher_to_dispatch_events) {
   // Given
   fake_event event;
   EXPECT_CALL(*target.ds, dispatch(_));
   std::allocator<fake_event> allocator;

   // When
   target.apply_change(allocator, std::move(event));

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(target.ds.get()));
}


TEST_F(artifact_test, has_uncommitted_events_is_0_when_newly_constructed) {
   // Given

   // When
   bool actual = target.has_uncommitted_events();

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(artifact_test, uncommitted_events_is_empty_when_newly_constructed) {
   // Given

   // When
   bool actual = target.uncommitted_events().empty();

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(artifact_test, add_handler_registers_event_handler) {
   // Given
   EXPECT_CALL(*target.ds, add_handler(_, _));

   // When
   target.handle<fake_event>([](fake_event){});

   // Then
   ASSERT_TRUE(Mock::VerifyAndClear(target.ds.get()));
}


TEST_F(artifact_test, revision_returns_0_without_applying_an_event) {
   // Given

   // When
   auto actual = target.revision();

   // Then
   ASSERT_EQ(0, actual);
}


TEST_F(artifact_test, has_uncommitted_events_returns_true_after_applying_an_event) {
   // Given
   fake_event e;
   target.apply_change(std::move(e));

   // When
   bool actual = target.has_uncommitted_events();

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(artifact_test, uncommitted_events_returns_a_sequence_with_a_single_event) {
   // Given
   fake_event e;
   target.apply_change(std::move(e));

   // When
   auto actual = target.uncommitted_events() >> std::experimental::count();

   // Then
   ASSERT_EQ(1, actual);
}


TEST_F(artifact_test, has_uncommitted_events_returns_false_after_applying_an_event_and_clearing) {
   // Given
   fake_event e;
   target.apply_change(std::move(e));
   target.clear_uncommitted_events();

   // When
   bool actual = target.has_uncommitted_events();

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(artifact_test, uncommitted_events_returns_a_sequence_without_any_events_after_clearing) {
   // Given
   fake_event e;
   target.apply_change(std::move(e));
   target.clear_uncommitted_events();

   // When
   auto actual = target.uncommitted_events() >> std::experimental::count();

   // Then
   ASSERT_EQ(0, actual);
}

}
