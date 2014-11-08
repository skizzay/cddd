#ifndef CDDD_CQRS_TRAITS_H__
#define CDDD_CQRS_TRAITS_H__

#include "cqrs/artifact_traits.h"
#include <sequence.h>
#include <type_traits>
#include <tuple>


namespace cddd {
namespace cqrs {

template<class T>
struct function_traits {
   typedef typename function_traits<decltype(&T::operator())>::result_type result_type;
   typedef typename function_traits<decltype(&T::operator())>::argument_type argument_type;
};


template<class R, class C, class... Args>
struct function_traits<R (C::*)(Args...)> {
   typedef R result_type;
   typedef std::tuple<Args...> argument_type;
};


template<class R, class C, class A>
struct function_traits<R (C::*)(A)> {
   typedef R result_type;
   typedef A argument_type;
};


template<class R, class C, class... Args>
struct function_traits<R (C::*)(Args...) const> {
   typedef R result_type;
   typedef std::tuple<Args...> argument_type;
};


template<class R, class C, class A>
struct function_traits<R (C::*)(A) const> {
   typedef R result_type;
   typedef A argument_type;
};


template<class R, class... Args>
struct function_traits<R (*)(Args...)> {
   typedef R result_type;
   typedef std::tuple<Args...> argument_type;
};


template<class R, class A>
struct function_traits<R (*)(A)> {
   typedef R result_type;
   typedef A argument_type;
};

#define HAS_MEMBER_FUNC(f_name) \
   template<class, class> struct has_##f_name; \
   template<class, class> struct has_const_##f_name; \
   namespace details_ { \
   template<class T, class Return, class... Args> \
   inline decltype(std::declval<T>().f_name(std::declval<Args>()...), std::is_same<Return, decltype(std::declval<T>().f_name(std::declval<Args>()...))>{}) test_##f_name(int) { return std::true_type{}; } \
   template<class T, class Return, class... Args> \
   inline std::false_type test_##f_name(long) { return std::false_type{}; } } \
   template<class T, class Return, class... Args> struct has_##f_name<T, Return(Args...)> : decltype(cddd::cqrs::details_::test_##f_name<T, Return, Args...>(0)) {}; \
   template<class T, class Return, class... Args> struct has_const_##f_name<T, Return(Args...)> : decltype(details_::test_##f_name<std::add_const_t<T>, Return, Args...>(0)) {}

#define HAS_TYPEDEF(t_name) \
   namespace details_ { \
   template<class T> \
   inline decltype(std::declval<typename T::t_name>(), std::true_type{}) test_has_typedef_for_##t_name(T &&) { return std::true_type{}; }\
   template<class T> \
   inline std::false_type test_has_typedef_for_##t_name(...) { return std::false_type{}; } } \
   template<class T> struct has_typedef_##t_name : decltype(details_::test_has_typedef_for_##t_name(std::declval<T>())) {}


HAS_MEMBER_FUNC(load);
HAS_MEMBER_FUNC(save);
HAS_MEMBER_FUNC(has);
HAS_MEMBER_FUNC(get);
HAS_MEMBER_FUNC(put);
HAS_TYPEDEF(value_type);
HAS_TYPEDEF(stream_type);
HAS_TYPEDEF(pointer);


template<class T>
struct is_stream : std::integral_constant<bool, has_typedef_value_type<T>::value &&
                                                has_typedef_pointer<T>::value &&
                                                has_save<T, void(std::experimental::sequence<typename T::pointer>)>::value &&
                                                has_const_load<T, std::experimental::sequence<typename T::pointer>()>::value> {};


template<class T>
struct is_store : std::integral_constant<bool, has_typedef_value_type<T>::value &&
                                               has_typedef_pointer<T>::value &&
                                               has_const_get<T, typename T::pointer(object_id)>::value &&
                                               has_put<T, void(typename T::pointer)>::value &&
                                               has_const_has<T, bool(object_id)>::value> {};


template<class T>
struct is_repository : std::integral_constant<bool, is_stream<T>::value && is_store<T>::value> {};


template<class T>
struct is_source : std::integral_constant<bool, is_store<T>::value &&
                                                has_typedef_stream_type<T>::value &&
                                                is_stream<typename T::stream_type>::value> {};

}
}

#endif
