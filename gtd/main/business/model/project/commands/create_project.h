//
// Created by andrew on 2/16/24.
//

#pragma once

#include <skizzay/cqrs/command.h>

namespace gtd {

struct create_project final : skizzay::simple::cqrs::command_base<create_project> {
    skizzay::simple::cqrs::uuid owner_id;
    std::string name;
};

} // gtd