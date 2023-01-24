#pragma once

#include "skizzay/cddd/commit_failed.h"

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

template <std::derived_from<std::exception> E, typename T>
requires requires(T const &t) {
  { t.GetErrorType() } -> operation_failed_error_details_::enum_value;
  { t.GetMessage() } -> std::same_as<Aws::String const &>;
}
struct operation_failed_error : E {
  using error_type =
      std::remove_cvref_t<decltype(std::declval<T const &>().GetErrorType())>;

  operation_failed_error(T const &t)
      : E{t.GetMessage()}, error_{t.GetErrorType()} {}

  error_type error() const noexcept { return error_; }

private:
  error_type error_;
};

template <typename T>
using commit_error = operation_failed_error<commit_failed, T>;
} // namespace skizzay::cddd::dynamodb
