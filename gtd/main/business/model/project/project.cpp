//
// Created by andrew on 2/16/24.
//

#include "project.h"

#include "commands/add_task_to_project.h"
#include "commands/cancel_project.h"
#include "commands/create_project.h"
#include "commands/mark_project_task_as_completed.h"
#include "commands/reactivate_task_within_project.h"
#include "commands/remove_task_from_project.h"
#include "events/project_reactivated.h"
#include "events/project_task_completed.h"
#include "events/project_task_reactivated.h"

namespace gtd {
    void project::uninitialized::validate_command(skizzay::simple::cqrs::uuid const &aggregate_id,
                                                  std::uint64_t const version, create_project const &command
    ) const {
        if (aggregate_id != command.aggregate_id) {
            throw std::invalid_argument{"Aggregate ID does not match project ID"};
        }
        if (version != 0) {
            throw std::invalid_argument{"Aggregate version is not 0"};
        }
        if (command.name.empty()) {
            throw std::invalid_argument{"Project name is empty"};
        }
        if (skizzay::simple::cqrs::uuid::nil() == command.owner_id) {
            throw std::invalid_argument{"Owner ID is nil"};
        }
    }

    project::event_result_set project::uninitialized::calculate_changes(create_project const &command) const {
        return event_result_set{project_created{{}, command.owner_id, command.name}};
    }

    project::active project::uninitialized::on_event(project_created const &) {
        return active{};
    }

    void project::active::validate_command(skizzay::simple::cqrs::uuid const &aggregate_id, std::uint64_t const version,
                                           add_task_to_project const &command
    ) const {
        if (aggregate_id != command.aggregate_id) {
            throw std::invalid_argument{"Aggregate ID does not match project ID"};
        }
        if (version == 0) {
            throw std::invalid_argument{"Aggregate version is 0"};
        }
        if (skizzay::simple::cqrs::uuid::nil() == command.task_id) {
            throw std::invalid_argument{"Task ID is nil"};
        }
        if (incomplete_task_ids.contains(command.task_id) || completed_task_ids.contains(command.task_id)) {
            throw std::invalid_argument{"Task already added to project"};
        }
    }

    project::event_result_set project::active::calculate_changes(add_task_to_project const &command) const {
        return event_result_set{project_added_task{{}, command.task_id, command.is_completed_task}};
    }

    void project::active::validate_command(skizzay::simple::cqrs::uuid const &aggregate_id, std::uint64_t version,
                                           remove_task_from_project const &command
    ) const {
        if (aggregate_id != command.aggregate_id) {
            throw std::invalid_argument{"Aggregate ID does not match project ID"};
        }
        if (version == 0) {
            throw std::invalid_argument{"Aggregate version is 0"};
        }
        if (skizzay::simple::cqrs::uuid::nil() == command.task_id) {
            throw std::invalid_argument{"Task ID is nil"};
        }
        if (!(incomplete_task_ids.contains(command.task_id) || completed_task_ids.contains(command.task_id))) {
            throw std::invalid_argument{"Task not found in project"};
        }
    }

    project::event_result_set project::active::calculate_changes(remove_task_from_project const &command) const {
        bool const is_completed_task = completed_task_ids.contains(command.task_id);
        event_result_set changes;
        changes.reserve(2);
        changes.emplace_back(project_removed_task{{}, command.task_id, is_completed_task});
        if (is_completed_task && completed_task_ids.size() == 1) {
            changes.emplace_back(project_completed{{}, project_completion_reason::all_tasks_completed});
        }
        return changes;
    }

    void project::active::validate_command(skizzay::simple::cqrs::uuid const &aggregate_id, std::uint64_t version,
        reactivate_task_within_project const &command
        ) const {
        if (aggregate_id != command.aggregate_id) {
            throw std::invalid_argument{"Aggregate ID does not match project ID"};
        }
        if (version == 0) {
            throw std::invalid_argument{"Aggregate version is 0"};
        }
        if (skizzay::simple::cqrs::uuid::nil() == command.task_id) {
            throw std::invalid_argument{"Task ID is nil"};
        }
        if (incomplete_task_ids.contains(command.task_id)) {
            throw std::invalid_argument{"Task already active"};
        }
        if (!completed_task_ids.contains(command.task_id)) {
            throw std::invalid_argument{"Task not found in project"};
        }
    }

    project::event_result_set project::active::calculate_changes(reactivate_task_within_project const &command) const {
        return event_result_set{project_task_reactivated{{}, command.task_id}};
    }

    void project::active::validate_command(skizzay::simple::cqrs::uuid const &aggregate_id, std::uint64_t const version,
                                           cancel_project const &command
    ) const {
        if (aggregate_id != command.aggregate_id) {
            throw std::invalid_argument{"Aggregate ID does not match project ID"};
        }
        if (version == 0) {
            throw std::invalid_argument{"Aggregate version is 0"};
        }
    }

    project::event_result_set project::active::calculate_changes(cancel_project const &) const {
        return event_result_set{project_completed{{}, project_completion_reason::cancelled}};
    }

    void project::active::validate_command(skizzay::simple::cqrs::uuid const &aggregate_id, std::uint64_t version,
                                           mark_project_task_as_completed const &command
    ) const {
        if (aggregate_id != command.aggregate_id) {
            throw std::invalid_argument{"Aggregate ID does not match project ID"};
        }
        if (version == 0) {
            throw std::invalid_argument{"Aggregate version is 0"};
        }
        if (skizzay::simple::cqrs::uuid::nil() == command.task_id) {
            throw std::invalid_argument{"Task ID is nil"};
        }
        if (completed_task_ids.contains(command.task_id)) {
            throw std::invalid_argument{"Task already completed"};
        }
        if (!incomplete_task_ids.contains(command.task_id)) {
            throw std::invalid_argument{"Task not found in project"};
        }
    }

    project::event_result_set project::active::calculate_changes(mark_project_task_as_completed const &command) const {
        event_result_set changes;
        changes.reserve(2);
        changes.emplace_back(project_task_completed{{}, command.task_id});
        if (incomplete_task_ids.size() == 1) {
            changes.emplace_back(project_completed{{}, project_completion_reason::all_tasks_completed});
        }
        return changes;
    }

    project::active project::active::on_event(project_added_task const &event) {
        if (event.is_completed_task) {
            completed_task_ids.insert(event.task_id);
        }
        else {
            incomplete_task_ids.insert(event.task_id);
        }
        return *this;
    }

    project::active project::active::on_event(project_removed_task const &event) {
        if (event.is_completed_task) {
            completed_task_ids.erase(event.task_id);
        }
        else {
            incomplete_task_ids.erase(event.task_id);
        }
        return *this;
    }

    project::active project::active::on_event(project_task_completed const &event) {
        incomplete_task_ids.erase(event.task_id);
        completed_task_ids.insert(event.task_id);
        return *this;
    }

    project::active project::active::on_event(project_task_reactivated const &event) {
        incomplete_task_ids.insert(event.task_id);
        completed_task_ids.erase(event.task_id);
        return *this;
    }

    void project::completed::validate_command(skizzay::simple::cqrs::uuid const &aggregate_id,
                                              std::uint64_t const version,
                                              add_task_to_project const &command
    ) const {
        if (aggregate_id != command.aggregate_id) {
            throw std::invalid_argument{"Aggregate ID does not match project ID"};
        }
        if (version == 0) {
            throw std::invalid_argument{"Aggregate version is 0"};
        }
        if (skizzay::simple::cqrs::uuid::nil() == command.task_id) {
            throw std::invalid_argument{"Task ID is nil"};
        }
        if (incomplete_task_ids.contains(command.task_id) || completed_task_ids.contains(command.task_id)) {
            throw std::invalid_argument{"Task already added to project"};
        }
    }

    project::event_result_set project::completed::calculate_changes(add_task_to_project const &command) const {
        if (command.is_completed_task) {
            return event_result_set{project_added_task{{}, command.task_id, false}};
        }
        return event_result_set{
            project_reactivated{{}, project_reactivation_reason::task_added},
            project_added_task{{}, command.task_id, true}
        };
    }

    void project::completed::validate_command(skizzay::simple::cqrs::uuid const &aggregate_id,
                                              std::uint64_t const version,
                                              remove_task_from_project const &command
    ) const {
        if (aggregate_id != command.aggregate_id) {
            throw std::invalid_argument{"Aggregate ID does not match project ID"};
        }
        if (version == 0) {
            throw std::invalid_argument{"Aggregate version is 0"};
        }
        if (skizzay::simple::cqrs::uuid::nil() == command.task_id) {
            throw std::invalid_argument{"Task ID is nil"};
        }
        if (!(incomplete_task_ids.contains(command.task_id) || completed_task_ids.contains(command.task_id))) {
            throw std::invalid_argument{"Task not found in project"};
        }
    }

    project::event_result_set project::completed::calculate_changes(remove_task_from_project const &command) const {
        return event_result_set{
            project_removed_task{{}, command.task_id, completed_task_ids.contains(command.task_id)}
        };
    }

    void project::completed::validate_command(skizzay::simple::cqrs::uuid const &aggregate_id, std::uint64_t version,
        reactivate_task_within_project const &command
        ) const {
        if (aggregate_id != command.aggregate_id) {
            throw std::invalid_argument{"Aggregate ID does not match project ID"};
        }
        if (version == 0) {
            throw std::invalid_argument{"Aggregate version is 0"};
        }
        if (skizzay::simple::cqrs::uuid::nil() == command.task_id) {
            throw std::invalid_argument{"Task ID is nil"};
        }
        if (incomplete_task_ids.contains(command.task_id)) {
            throw std::invalid_argument{"Task already active"};
        }
        if (!completed_task_ids.contains(command.task_id)) {
            throw std::invalid_argument{"Task not found in project"};
        }
    }

    project::event_result_set project::completed::calculate_changes(reactivate_task_within_project const &command) const {
        return event_result_set{
            project_reactivated{{}, project_reactivation_reason::task_reactivated},
            project_task_reactivated{{}, command.task_id}
        };
    }

    project::completed project::completed::on_event(project_added_task const &event) {
        completed_task_ids.insert(event.task_id);
        return *this;
    }

    project::active project::completed::on_event(project_reactivated const &) {
        return active{{}, std::move(incomplete_task_ids), std::move(completed_task_ids)};
    }

    project::completed project::active::on_event(project_completed const &) {
        return completed{{}, std::move(incomplete_task_ids), std::move(completed_task_ids)};
    }
} // gtd
