//
// Created by andrew on 2/13/24.
//

#pragma once

#include <type_traits>

namespace skizzay::s11n {
    template<typename>
    struct is_boolean : std::false_type {
    };

    namespace boolean_details_ {
        template<typename T>
        concept boolean_testable = std::is_convertible_v<T, bool>;
    }

    template<typename T>
        requires boolean_details_::boolean_testable<T> && requires(T &&t) {
            { !static_cast<T &&>(t) } -> boolean_details_::boolean_testable;
        }
    struct is_boolean<T> : std::true_type {
    };

    template<typename T>
    concept boolean = is_boolean<T>::value;
} // skizzay
