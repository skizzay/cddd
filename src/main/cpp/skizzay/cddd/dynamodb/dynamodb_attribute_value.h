#pragma once

#include "skizzay/cddd/timestamp.h"
#include <aws/dynamodb/model/AttributeValue.h>
#include <chrono>
#include <concepts>
#include <type_traits>

namespace skizzay::cddd::dynamodb {

inline Aws::DynamoDB::Model::AttributeValue attribute_value(bool const value) {
  return Aws::DynamoDB::Model::AttributeValue{}.SetB(value);
}

inline Aws::DynamoDB::Model::AttributeValue attribute_value(int const value) {
  return Aws::DynamoDB::Model::AttributeValue{}.SetN(value);
}

inline Aws::DynamoDB::Model::AttributeValue
attribute_value(std::integral auto const value) {
  return attribute_value(narrow_cast<int>(value));
}

inline Aws::DynamoDB::Model::AttributeValue
attribute_value(std::string_view const value) {
  return Aws::DynamoDB::Model::AttributeValue{}.SetS(
      Aws::String{value.data(), value.size()});
}

inline Aws::DynamoDB::Model::AttributeValue
attribute_value(std::floating_point auto const value) {
  return Aws::DynamoDB::Model::AttributeValue{}.SetN(value);
}

template <typename T>
requires std::is_constructible_v < Aws::DynamoDB::Model::AttributeValue,
    std::remove_reference_t<T>
const & > inline Aws::DynamoDB::Model::AttributeValue
          attribute_value(T const &value) {
  return Aws::DynamoDB::Model::AttributeValue{value};
}

namespace timestamp_details_ {
template <typename Duration>
inline constexpr char const *format_expression = "";

template <>
inline constexpr char const *format_expression<std::chrono::seconds> =
    "%04d-%02d-%02dT%02d:%02d:%02dZ";

template <>
inline constexpr char const *format_expression<std::chrono::milliseconds> =
    "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ";

template <>
inline constexpr char const *format_expression<std::chrono::microseconds> =
    "%04d-%02d-%02dT%02d:%02d:%02d.%06dZ";

template <>
inline constexpr char const *format_expression<std::chrono::nanoseconds> =
    "%04d-%02d-%02dT%02d:%02d:%02d.%09dZ";

template <typename Duration>
Aws::DynamoDB::Model::AttributeValue
to_string_value(std::chrono::year_month_day ymd,
                std::chrono::hh_mm_ss<Duration> tod) {
  constexpr std::size_t buffer_size = 32;
  int n = 0, year = ymd.year(), month = ymd.month(), day = ymd.day();
  std::array<char, buffer_size> buffer;
  if constexpr (std::is_same_v<Duration, std::chrono::seconds>) {
    n = std::snprintf(
        buffer.data(), buffer_size,
        timestamp_details_::format_expression<std::chrono::seconds>, year,
        month, day, tod.hours().count(), tod.minutes().count(),
        tod.seconds().count());
  } else if (0 == tod.subseconds().count()) {
    n = std::snprintf(
        buffer.data(), buffer_size,
        timestamp_details_::format_expression<std::chrono::seconds>, year,
        month, day, tod.hours().count(), tod.minutes().count(),
        tod.seconds().count());
  } else {
    n = std::snprintf(buffer.data(), buffer_size,
                      timestamp_details_::format_expression<Duration>, year,
                      month, day, tod.hours().count(), tod.minutes().count(),
                      tod.seconds().count(),
                      narrow_cast<int>(tod.subseconds().count()));
  }
  return attribute_value(Aws::String{buffer.data(), n});
}
} // namespace timestamp_details_

template <typename Duration>
inline Aws::DynamoDB::Model::AttributeValue
attribute_value(std::chrono::utc_time<Duration> const timestamp) {
  auto date = std::chrono::floor<std::chrono::days>(timestamp);
  return timestamp_details_::to_string_value(
      std::chrono::year_month_day{date},
      std::chrono::hh_mm_ss{timestamp - date});
}

inline Aws::DynamoDB::Model::AttributeValue
attribute_value(concepts::timestamp auto const timestamp) {
  std::array<char, 32> buffer;
  std::size_t const n = std::snprintf(buffer.data(), buffer.size(), "%zd", timestamp.time_since_epoch().count());
  return Aws::DynamoDB::Model::AttributeValue{}.SetN(
      Aws::String{buffer.data(), n});
}

inline Aws::DynamoDB::Model::AttributeValue attribute_value(
    Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> subobject) {
  Aws::DynamoDB::Model::AttributeValue result;
  auto move_to_result = [&result](Aws::String const &key,
                                  Aws::DynamoDB::Model::AttributeValue &value) {
    result.AddMEntry(key,
                     std::make_shared<Aws::DynamoDB::Model::AttributeValue>(
                         std::move(value)));
  };
  std::ranges::for_each(subobject, [move_to_result](auto &subitem) {
    std::apply(move_to_result, subitem);
  });
  return result;
}

} // namespace skizzay::cddd::dynamodb
