//
// Created by andrew on 2/19/24.
//

#include <catch2/catch_all.hpp>
#include <business/model/project/project.h>

#include "business/model/project/commands/create_project.h"
#include "business/model/project/commands/add_task_to_project.h"
#include "business/model/project/commands/mark_project_task_as_completed.h"

using namespace gtd;

TEST_CASE("lifecycle of a project", "[project,command_processing]") {
    // Arrange
    auto const project_id = skizzay::simple::cqrs::uuid::v4();
    auto target = project::aggregate_root_type{project_id};

    // Assert
    REQUIRE(target.id() == project_id);
    REQUIRE(target.uncommitted_events().empty());
    REQUIRE(target.is_state<project::uninitialized>());
    REQUIRE(target.version() == 0);

    SECTION("create project") {
        // Arrange
        auto const owner_id = skizzay::simple::cqrs::uuid::v4();
        auto const project_name = "Test Project";

        // Act
        target.handle_command(create_project{{project_id, std::chrono::system_clock::now()}, owner_id, project_name});
        auto committed_events = std::move(target).commit();

        // Assert
        REQUIRE(target.id() == project_id);
        REQUIRE(target.is_state<project::active>());
        REQUIRE(committed_events.size() == 1);

        SECTION("add tasks to the project") {
            // Arrange
            auto const task_ids = std::array{skizzay::simple::cqrs::uuid::v4(), skizzay::simple::cqrs::uuid::v4()};

            // Act
            target.handle_command(add_task_to_project{{project_id, std::chrono::system_clock::now()}, task_ids[0]});
            target.handle_command(add_task_to_project{{project_id, std::chrono::system_clock::now()}, task_ids[1]});
            committed_events = std::move(target).commit();

            // Assert
            REQUIRE(target.id() == project_id);
            REQUIRE(committed_events.size() == 2);
            target.query([&task_ids]<typename State>(State const &state) {
                if constexpr (std::is_same_v<State, project::active>) {
                    REQUIRE(state.incomplete_task_ids.size() == task_ids.size());
                    REQUIRE(state.completed_task_ids.empty());
                    for (auto const &task_id: task_ids) {
                        REQUIRE(state.incomplete_task_ids.contains(task_id));
                    }
                }
                else {
                    FAIL("Unexpected state");
                }
            });

            SECTION("complete task") {
                // Arrange
                auto const task_id = task_ids[0];

                // Act
                target.handle_command(mark_project_task_as_completed{
                    {project_id, std::chrono::system_clock::now()}, task_id
                });
                committed_events = std::move(target).commit();

                // Assert
                REQUIRE(target.id() == project_id);
                REQUIRE(committed_events.size() == 1);
                target.query([&task_id, &task_ids]<typename State>(State const &state) {
                    if constexpr (std::is_same_v<State, project::active>) {
                        REQUIRE(state.incomplete_task_ids.size() == task_ids.size() - 1);
                        REQUIRE(state.completed_task_ids.size() == 1);
                        REQUIRE(state.incomplete_task_ids.contains(task_ids[1]));
                        REQUIRE(state.completed_task_ids.contains(task_id));
                    }
                    else {
                        FAIL("Unexpected state");
                    }
                });

                // Part of happy path
                SECTION("complete final task") {
                    // Arrange
                    auto const final_task_id = task_ids[1];

                    // Act
                    target.handle_command(mark_project_task_as_completed{
                        {project_id, std::chrono::system_clock::now()}, final_task_id
                    });
                    committed_events = std::move(target).commit();

                    // Assert
                    REQUIRE(target.id() == project_id);
                    REQUIRE(committed_events.size() == 2);
                    target.query([&task_ids]<typename State>(State const &state) {
                        if constexpr (std::is_same_v<State, project::completed>) {
                            REQUIRE(state.incomplete_task_ids.empty());
                            REQUIRE(state.completed_task_ids.size() == task_ids.size());
                            for (auto const &task_id: task_ids) {
                                REQUIRE(state.completed_task_ids.contains(task_id));
                            }
                        }
                        else {
                            FAIL("Unexpected state");
                        }
                    });
                }
            }
        }
    }
}
