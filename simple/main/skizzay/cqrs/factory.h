//
// Created by andrew on 1/23/24.
//

#ifndef FACTORY_H
#define FACTORY_H
#include <type_traits>

namespace skizzay::simple::cqrs {
    template<typename F, typename R, typename... Args>
    concept factory = std::is_invocable_r_v<R, F, Args...>;

    template<typename R = void>
    struct default_factory final {
        template<typename R2=R, typename... Args>
        constexpr R2 operator()(Args &&... args) const noexcept(std::is_nothrow_constructible_v<R, Args...>) requires std::constructible_from<R2, Args...> {
            return R2{std::forward<Args>(args)...};
        }

        constexpr void operator()(auto &&... ) const noexcept requires std::same_as<R, void> {
        }
    };

    template<std::copy_constructible T>
    struct prototype_factory final {
        template<typename... Args>
        explicit constexpr prototype_factory(Args &&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) requires std::constructible_from<T, Args...>
            : prototype_{std::forward<Args>(args)...} {
        }

        constexpr T operator()() const noexcept(std::is_nothrow_copy_constructible_v<T>) {
            return prototype_;
        }

    private:
        T prototype_;
    };
}

#endif //FACTORY_H
