#pragma once

#include <aws/core/client/AWSError.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <concepts>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace skizzay::cddd::dynamodb {

namespace operation_failed_error_details_ {
template <typename T>
concept enum_value = std::is_enum_v<T>;
} // namespace operation_failed_error_details_

struct basic_operation_failed_error : std::runtime_error {
  template <typename T>
  requires requires(T const &tc) { {std::runtime_error{tc.GetMessage()}}; }
  explicit basic_operation_failed_error(T const &t)
      : std::runtime_error{t.GetMessage()} {}
};

template <typename T>
requires requires(T const &t) {
  { t.GetErrorType() } -> operation_failed_error_details_::enum_value;
  { t.GetMessage() } -> std::same_as<Aws::String const &>;
}
struct operation_failed_error : basic_operation_failed_error {
  using error_type =
      std::remove_cvref_t<decltype(std::declval<T const &>().GetErrorType())>;

  operation_failed_error(T const &t)
      : basic_operation_failed_error{t}, error_{t.GetErrorType()} {}

  error_type error() const noexcept { return error_; }

private:
  error_type error_;
};

} // namespace skizzay::cddd::dynamodb
