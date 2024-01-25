//
// Created by andrew on 1/19/24.
//

#ifndef SIMPLE_TRAITS_H
#define SIMPLE_TRAITS_H

#include <type_traits>

namespace skizzay::simple::cqrs {
    template<typename, template<typename ...> typename>
    struct is_template : std::false_type {
    };

    template<template<typename ...> typename T, typename ...Args>
    struct is_template<T<Args...>, T> : std::true_type {
    };

    template<typename T, template<typename ...> typename Template>
    inline constexpr bool is_template_v = is_template<T, Template>::value;

    template<typename T, template<typename ...> typename Template>
    concept template_of = is_template_v<T, Template>;
} // skizzay

#endif //SIMPLE_TRAITS_H
