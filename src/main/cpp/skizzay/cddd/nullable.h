#pragma once

#include <concepts>
#include <memory>
#include <optional>
#include <type_traits>
#include <variant>

namespace skizzay::cddd {
template <typename T>
inline constexpr std::optional<T> null_value = std::nullopt;

template <typename T>
inline constexpr std::optional<T> null_value<std::optional<T>> = std::nullopt;

template <typename T> inline constexpr T const *null_value<T *> = nullptr;

template <typename T>
inline std::shared_ptr<T> const null_value<std::shared_ptr<T>> = {};

template <typename T, typename D>
inline constexpr std::unique_ptr<T, D> null_value<std::unique_ptr<T, D>> = {};

template <typename... Ts>
requires(
    std::same_as<Ts, std::monostate> ||
    ...) inline constexpr std::variant<Ts...> null_value<std::variant<Ts...>> =
    std::monostate{};

template <typename T>
using nullable_t = std::remove_const_t<decltype(null_value<T>)>;

namespace concepts {
template <typename T>
concept nullable = std::same_as<T, nullable_t<T>>;
}
} // namespace skizzay::cddd
