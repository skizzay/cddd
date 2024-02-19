//
// Created by andrew on 2/16/24.
//

#pragma once

#include <stdexcept>

namespace skizzay::simple::cqrs {
    // ReSharper disable once CppClassCanBeFinal
    struct unexpected_command : std::logic_error {
        using std::logic_error::logic_error;
    };

    // ReSharper disable once CppClassCanBeFinal
    struct unexpected_event : std::logic_error {
        using std::logic_error::logic_error;
    };
} // skizzay
