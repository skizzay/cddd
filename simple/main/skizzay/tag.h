//
// Created by andrew on 1/24/24.
//

#ifndef TAG_H
#define TAG_H

#include <type_traits>

namespace skizzay::simple::cqrs {

    template<auto T>
    using tag_t = std::decay_t<decltype(T)>;

} // namespace skizzay::simple::cqrs

#endif //TAG_H
