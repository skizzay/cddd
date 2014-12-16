#ifndef CDDD_UTILS_FUNCTION_TRAITS_H__
#define CDDD_UTILS_FUNCTION_TRAITS_H__

#include <type_traits>

namespace cddd {
namespace utils {

template<class> struct function_traits;
template<class... Args> struct parameter_pack {
};


namespace details_ {

template<std::size_t, class> struct argument_at;

template<std::size_t I, class Head, class... Tail>
struct argument_at<I, parameter_pack<Head, Tail...>> : argument_at<I - 1, parameter_pack<Tail...>> {
};

template<class Head, class... Tail>
struct argument_at<0, parameter_pack<Head, Tail...>> {
   typedef Head type;
};

}


template<class R, class... Args>
struct function_traits<R(Args...)> {
   typedef R result_type;
   typedef parameter_pack<Args...> argument_types;

   enum { arity = sizeof...(Args) };

   template<std::size_t I, class=std::enable_if<(I < arity)>>
   struct argument {
      typedef typename details_::argument_at<I, argument_types>::type type;
   };
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

#endif
