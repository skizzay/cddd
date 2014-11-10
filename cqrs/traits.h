#ifndef CDDD_CQRS_TRAITS_H__
#define CDDD_CQRS_TRAITS_H__

#include "cqrs/artifact_traits.h"
#include "cqrs/function_traits.h"
#include "cqrs/pointer_traits.h"
#include <type_traits>


namespace cddd {
namespace cqrs {

#if 0
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
   template<class T, typename=void> \
   inline std::false_type test_has_typedef_for_##t_name(); \
   template<class T, typename=typename T::memento_type> \
   inline std::true_type test_has_typedef_for_##t_name(); \
   template<class T> struct has_typedef_##t_name : decltype(details_::test_has_typedef_for_##t_name<T>()) {}

HAS_MEMBER_FUNC(apply_memento);
HAS_MEMBER_FUNC(get_memento);
HAS_TYPEDEF(memento_type);

namespace details_ {

template<class, bool> struct is_snapshotable_helper;

template<class T>
struct is_snapshotable_helper<T, true> : std::integral_constant<bool, has_const_get_memento<T, typename T::memento_type()> &&
                                                                      has_apply_memento<T, void(const typename T::memento_type&)>> {
};

template<class T>
struct is_snapshotable_helper<T, false> : std::false_type {
};

}

template<class T>
struct is_snapshotable : details_::is_snapshotable_helper<T, has_typedef_memento_type<T>> {
};
#endif

}
}

#endif
