//
// Created by andrew on 2/18/24.
//

#pragma once

#include "project_added_task.h"
#include "project_completed.h"
#include "project_created.h"
#include "project_reactivated.h"
#include "project_removed_task.h"
#include "project_task_completed.h"
#include "project_task_reactivated.h"
#include <variant>

namespace gtd {
    using project_event = std::variant<project_created, project_completed, project_added_task, project_removed_task,
        project_reactivated, project_task_completed, project_task_reactivated>;
} // gtd
