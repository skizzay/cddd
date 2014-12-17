#ifndef CDDD_MESSAGING_TRAITS_H__
#define CDDD_MESSAGING_TRAITS_H__

#include "utils/function_traits.h"
#include "utils/type_id_generator.h"

namespace cddd {
namespace messaging {

typedef utils::type_id_generator::type_id message_type_id;


template<class F> using message_from_argument = std::decay_t<typename utils::function_traits<F>::template argument<0>::type>;
template<class F> using message_from_result = std::decay_t<typename utils::function_traits<F>::result_type>;

}
}

#endif
