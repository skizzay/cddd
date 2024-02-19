//
// Created by andrew on 2/17/24.
//

#pragma once

#include <skizzay/cqrs/command.h>

namespace gtd {

struct cancel_project final : skizzay::simple::cqrs::command_base<cancel_project> {
};

} // gtd