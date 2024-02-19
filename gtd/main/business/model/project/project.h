//
// Created by andrew on 2/16/24.
//

#pragma once

#include <variant>
#include <vector>
#include <unordered_set>

#include <skizzay/cqrs/aggregate_root.h>
#include <skizzay/cqrs/state.h>
#include <skizzay/cqrs/state_machine.h>

#include "events/project_event.h"

namespace gtd {
    struct reactivate_task_within_project;
}

namespace gtd {
    struct remove_task_from_project;
}

namespace gtd {

    struct add_task_to_project;
    struct cancel_project;
    struct create_project;
    struct mark_project_task_as_completed;
    struct reactivate_task_within_project;
    struct remove_task_from_project;

    class project {
        using event_result_set = std::vector<project_event>;

    public:
        struct uninitialized;
        struct active;
        struct completed;

        using state_type = std::variant<uninitialized, active, completed>;

        struct uninitialized final : skizzay::simple::cqrs::state_base<uninitialized> {
            using state_base::on_event;

            void validate_command(skizzay::simple::cqrs::uuid const &aggregate_id, std::uint64_t version,
                                  create_project const &command
            ) const;

            [[nodiscard]] event_result_set calculate_changes(create_project const &) const;

            active on_event(project_created const &);
        };

        struct active final : skizzay::simple::cqrs::state_base<active> {
            using state_base::on_event;

            std::unordered_set<skizzay::simple::cqrs::uuid> incomplete_task_ids{};
            std::unordered_set<skizzay::simple::cqrs::uuid> completed_task_ids{};

            void validate_command(skizzay::simple::cqrs::uuid const &aggregate_id, std::uint64_t version,
                                  add_task_to_project const &command
            ) const;

            [[nodiscard]] event_result_set calculate_changes(add_task_to_project const &) const;

            void validate_command(skizzay::simple::cqrs::uuid const &aggregate_id, std::uint64_t version,
                                  remove_task_from_project const &command
            ) const;

            [[nodiscard]] event_result_set calculate_changes(remove_task_from_project const &) const;

            void validate_command(skizzay::simple::cqrs::uuid const &aggregate_id, std::uint64_t version,
                                  reactivate_task_within_project const &command
            ) const;

            [[nodiscard]] event_result_set calculate_changes(reactivate_task_within_project const &) const;

            void validate_command(skizzay::simple::cqrs::uuid const &aggregate_id, std::uint64_t version,
                                  cancel_project const &command
            ) const;

            [[nodiscard]] event_result_set calculate_changes(cancel_project const &) const;

            void validate_command(skizzay::simple::cqrs::uuid const &aggregate_id, std::uint64_t version,
                                  mark_project_task_as_completed const &command
            ) const;

            [[nodiscard]] event_result_set calculate_changes(mark_project_task_as_completed const &) const;

            active on_event(project_added_task const &);

            active on_event(project_removed_task const &);

            active on_event(project_task_completed const &);

            active on_event(project_task_reactivated const &);

            completed on_event(project_completed const &);
        };

        struct completed final : skizzay::simple::cqrs::state_base<completed> {
            using state_base::on_event;

            std::unordered_set<skizzay::simple::cqrs::uuid> incomplete_task_ids{};
            std::unordered_set<skizzay::simple::cqrs::uuid> completed_task_ids{};

            void validate_command(skizzay::simple::cqrs::uuid const &aggregate_id, std::uint64_t version,
                                  add_task_to_project const &command
            ) const;

            [[nodiscard]] event_result_set calculate_changes(add_task_to_project const &) const;

            void validate_command(skizzay::simple::cqrs::uuid const &aggregate_id, std::uint64_t version,
                                  remove_task_from_project const &command
            ) const;

            [[nodiscard]] event_result_set calculate_changes(remove_task_from_project const &) const;

            void validate_command(skizzay::simple::cqrs::uuid const &aggregate_id, std::uint64_t version,
                                  reactivate_task_within_project const &command
            ) const;

            [[nodiscard]] event_result_set calculate_changes(reactivate_task_within_project const &) const;

            completed on_event(project_added_task const &);

            active on_event(project_reactivated const &);
        };

        using aggregate_root_type = skizzay::simple::cqrs::aggregate_root<project_event,
            skizzay::simple::cqrs::state_machine<uninitialized, active, completed> >;
    };
} // gtd
