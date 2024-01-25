//
// Created by andrew on 1/19/24.
//

#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <chrono>

#include "simple_traits.h"

namespace skizzay::simple::cqrs {
    template<typename T>
    struct is_time_point : is_template<T, std::chrono::time_point> {
    };

    template<typename T>
    constexpr inline bool is_time_point_v = is_time_point<T>::value;

    template<typename T>
    concept time_point = is_time_point_v<T>;
} // skizzay

#endif //TIMESTAMP_H
