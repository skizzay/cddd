#ifndef CDDD_CQRS_CONTINUATION_CONTEXT_H__
#define CDDD_CQRS_CONTINUATION_CONTEXT_H__

#include <functional>
#include <utility>


namespace cddd {
namespace cqrs {

template<class> class continuation_context;

namespace details_ {

template<class RT, class Fun>
struct continuation_type_traits {
   typedef result_type decltype(std::declval<Fun>().value()(std::declval<RT>().value()));
   typedef type continuation_context<result_type>;
};


template<class Arg>
struct context_execution {
   virtual ~context_execution() = default;
   virtual void fire(Arg arg) = 0;
};


template<class Arg, class Fun>
struct context_execution_impl : context_execution<Arg> {
   typedef continuation_type_traits<Arg, Fun>::result_type result_type;

   context_execution_impl(Fun &&fun, std::unique_ptr<std::context_execution<result_type>> next) :
      context_execution(),
      f(std::forward(fun)),
      next_context(std::move(next))
   {
   }
   virtual ~context_execution_impl() = default;

   virtual void fire(Arg arg) {
      result_type result = f(arg);
      if (next_context != nullptr) {
         next_context->fire(result);
      }
   }

   Fun f;
   std::unique_ptr<std::context_execution<result_type>> next_context;
};

}

template<class ResultType>
class continuation_context {
public:
   template<class Fun>
   auto then(Fun &&f) -> typename details_::continuation_type_traits<ResultType, Fun>::type {
   }
};

}
}

#endif
