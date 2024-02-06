//
// Created by andrew on 2/5/24.
//

#ifndef THROW_SYSTEM_ERROR_H
#define THROW_SYSTEM_ERROR_H

#include <system_error>

namespace skizzay::s11n {
    constexpr inline auto throw_system_error = [][[noreturn]](int const err = errno) {
        throw std::system_error{err, std::system_category()};
    };
}

#endif //THROW_SYSTEM_ERROR_H
