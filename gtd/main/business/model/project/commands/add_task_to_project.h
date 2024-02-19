//
// Created by andrew on 2/16/24.
//

#pragma once

#include <skizzay/cqrs/command.h>

namespace gtd {
    struct add_task_to_project final : skizzay::simple::cqrs::command_base<add_task_to_project> {
        skizzay::simple::cqrs::uuid task_id{};
        bool is_completed_task = false;
    };

    static_assert(skizzay::simple::cqrs::is_command_v<add_task_to_project>);
} // gtd
