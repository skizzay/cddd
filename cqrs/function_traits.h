#ifndef CDDD_CQRS_FUNCTION_TRAITS_H__
#define CDDD_CQRS_FUNCTION_TRAITS_H__

#include <tuple>


namespace cddd {
namespace cqrs {

template<class> struct function_traits;


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


template<class T>
struct function_traits {
   typedef typename function_traits<decltype(&T::operator())>::result_type result_type;
   typedef typename function_traits<decltype(&T::operator())>::argument_type argument_type;
};

}
}

#endif
