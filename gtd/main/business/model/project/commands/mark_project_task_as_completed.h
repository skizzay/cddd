//
// Created by andrew on 2/17/24.
//

#pragma once

#include <skizzay/cqrs/command.h>
#include <skizzay/cqrs/uuid.h>

namespace gtd {

struct mark_project_task_as_completed final : skizzay::simple::cqrs::command_base<mark_project_task_as_completed> {
    skizzay::simple::cqrs::uuid task_id;
};

} // gtd