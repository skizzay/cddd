//
// Created by andrew on 2/16/24.
//

#pragma once

#include <cstddef>

namespace skizzay::simple::cqrs { namespace type_sequence_details_ {
        template<std::size_t, typename...>
        struct at_impl;

        template<typename T, typename... Ts>
        struct at_impl<0, T, Ts...> {
            using type = T;
        };

        template<std::size_t I, typename T, typename... Ts>
        struct at_impl<I, T, Ts...> {
            using type = typename at_impl<I - 1, Ts...>::type;
        };

        template<template<typename...> typename, typename, typename...>
        struct concat_impl;

        template<template<typename...> typename Template, typename... Ts, typename... Us>
        struct concat_impl<Template, Template<Ts...>, Template<Us...> > {
            using type = Template<Ts..., Us...>;
        };

        template<template<typename...> typename Template, typename... Ts, typename... Us, typename... Vs>
        struct concat_impl<Template, Template<Ts...>, Template<Us...>, Vs...> {
            using type = typename concat_impl<Template, Template<Ts..., Us...>, Vs...>::type;
        };
    }

    template<typename... Ts>
    struct type_sequence {
        template<std::size_t I>
        using at_t = typename type_sequence_details_::at_impl<I, Ts...>::type;

        template<typename... OtherTypeSequences>
        using concat_t = typename type_sequence_details_::concat_impl<type_sequence, type_sequence<Ts...>,
            OtherTypeSequences...>::type;

        using common_type_t = std::common_type_t<Ts...>;
    };

    template<>
    struct type_sequence<> {
        template<typename... OtherTypeSequences>
        using concat_t = typename type_sequence_details_::concat_impl<type_sequence, type_sequence<>,
            OtherTypeSequences...>::type;
    };
} // skizzay
