#pragma once

#include <cstdint>

namespace cddd {
namespace utils {

template<class... Args>
struct parameter_pack {
   static constexpr std::size_t size = sizeof...(Args);
};


template<class, class> struct append_to_parameter_pack;

template<class T, class ... List>
struct append_to_parameter_pack<T, parameter_pack<List...>> {
   using type = parameter_pack<List..., T>;
};


template<class ...> struct reverse_parameter_pack;

template<>
struct reverse_parameter_pack<> {
   using type = parameter_pack<>;
};

template<class T, class ... List>
struct reverse_parameter_pack<T, List...> {
   using type = typename append_to_parameter_pack<
         T,
         typename reverse_parameter_pack<List...>::type
      >::type;
};


template<std::size_t, class> struct parameter_at;

template<std::size_t I, class Head, class ... Tail>
struct parameter_at<I, parameter_pack<Head, Tail...>> : parameter_at<I - 1, parameter_pack<Tail...>>{
};

template<class Head, class ... Tail>
struct parameter_at<0, parameter_pack<Head, Tail...>> {
   using type = Head;
};

}
}
