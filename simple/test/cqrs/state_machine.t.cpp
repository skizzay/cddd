//
// Created by andrew on 1/20/24.
//

#include <catch2/catch_all.hpp>
#include <skizzay/cqrs/state_machine.h>

using namespace skizzay::simple::cqrs;

namespace {
  template<int N>
  struct test_command : command_base<test_command<N>> {
    [[nodiscard]] static constexpr int value() noexcept { return N; }
  };

  template<int N>
  struct test_event : event_base<test_event<N>> {
    [[nodiscard]] static constexpr int value() noexcept { return N; }
  };

  using event_type = std::variant<test_event<1>, test_event<2>>;
  using event_vector = std::vector<event_type>;

  template<int N>
  struct test_state : state_base<test_state<N>> {
    [[nodiscard]] static constexpr int value() noexcept { return N; }
    event_vector events;

    template<int M>
    [[nodiscard]] constexpr event_vector calculate_changes(test_command<M> const &cmd) const noexcept {
      return {test_event<M>{command_aggregate_id(cmd)}};
    }

    template<int M>
    [[nodiscard]] constexpr auto on_event(test_event<M> const &) const noexcept {
      return test_state<M>{};
    }
  };
}

TEST_CASE("state machine can be queried", "[state_machine]") {
  // Arrange
  state_machine<test_state<1>, test_state<2>> const sm;

  // Act
  auto const result = sm.query([](auto const &s) { return s.value(); });

  // Assert
  REQUIRE(result == 1);
}

TEST_CASE("state machine's initial state is the first state specified", "[state_machine]") {
  // Arrange
  state_machine<test_state<1>, test_state<2>> const sm;

  // Act & Assert
  REQUIRE(sm.is_state<test_state<1>>());
}

TEST_CASE("state machine can be transitioned to a new state", "[state_machine]") {
  // Arrange
  state_machine<test_state<1>, test_state<2>> sm;

  // Act
  sm.on_event(test_event<2>{});

  // Assert
  REQUIRE(sm.is_state<test_state<2>>());
}

TEST_CASE("state machine can handle multiple events", "[state_machine]") {
  // Arrange
  state_machine<test_state<1>, test_state<2>> sm;

  // Act
  sm.on_event(test_event<2>{});
  sm.on_event(event_type{test_event<1>{}});

  // Assert
  REQUIRE(sm.is_state<test_state<1>>());
}