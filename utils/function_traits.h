#pragma once

#include "utils/parameter_pack.h"
#include <type_traits>

namespace cddd {
namespace utils {

template<class> struct function_traits;


template<class R, class... Args>
struct function_traits<R(Args...)> {
   typedef R result_type;
   typedef parameter_pack<Args...> argument_types;

   enum { arity = sizeof...(Args) };

   template<std::size_t I, class=std::enable_if<(I < arity)>>
   struct argument {
      using type = typename parameter_at<I, argument_types>::type;
   };
};


template<class R, class C, class... Args>
struct function_traits<R (C::*)(Args...)> : function_traits<R(Args...)> {
   typedef C class_type;
};


template<class R, class C, class... Args>
struct function_traits<R (C::*)(Args...) const> : function_traits<R(Args...)> {
   typedef std::add_const_t<C> class_type;
};


template<class R, class C, class... Args>
struct function_traits<R (C::*)(Args...) volatile> : function_traits<R(Args...)> {
   typedef std::add_volatile_t<C> class_type;
};


template<class R, class C, class... Args>
struct function_traits<R (C::*)(Args...) const volatile> : function_traits<R(Args...)> {
   typedef std::add_cv_t<C> class_type;
};


template<class R, class... Args>
struct function_traits<R (*)(Args...)> : function_traits<R(Args...)>{
};

template<class T> struct function_traits<T &> : function_traits<T> {};
template<class T> struct function_traits<T &&> : function_traits<T> {};
template<class T> struct function_traits<const T &> : function_traits<T> {};
template<class T> struct function_traits<const T &&> : function_traits<T> {};
template<class T> struct function_traits<volatile T &> : function_traits<T> {};
template<class T> struct function_traits<volatile T &&> : function_traits<T> {};
template<class T> struct function_traits<const volatile T &> : function_traits<T> {};
template<class T> struct function_traits<const volatile T &&> : function_traits<T> {};
template<class T> struct function_traits : function_traits<decltype(&T::operator())> {};

}
}
