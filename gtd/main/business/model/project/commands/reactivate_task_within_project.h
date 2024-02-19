//
// Created by andrew on 2/17/24.
//

#pragma once

#include <skizzay/cqrs/command.h>
#include <skizzay/cqrs/uuid.h>

namespace gtd {
    struct reactivate_task_within_project final : skizzay::simple::cqrs::command_base<reactivate_task_within_project> {
        skizzay::simple::cqrs::uuid task_id;

        [[nodiscard]] skizzay::simple::cqrs::uuid const &project_id() const noexcept {
            return aggregate_id;
        }
    };
} // gtd
