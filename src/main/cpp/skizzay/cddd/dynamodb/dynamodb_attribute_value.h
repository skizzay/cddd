#pragma once

#include "skizzay/cddd/dictionary.h"
#include "skizzay/cddd/timestamp.h"
#include <aws/core/utils/memory/stl/AWSMap.h>
#include <aws/dynamodb/model/AttributeValue.h>
#include <charconv>
#include <chrono>
#include <concepts>
#include <sstream>
#include <system_error>
#include <type_traits>

namespace skizzay::cddd::dynamodb {

inline Aws::DynamoDB::Model::AttributeValue attribute_value(bool const value) {
  return Aws::DynamoDB::Model::AttributeValue{}.SetB(value);
}

inline Aws::DynamoDB::Model::AttributeValue
attribute_value(std::integral auto const value) {
  return Aws::DynamoDB::Model::AttributeValue{}.SetN(std::to_string(value));
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

inline Aws::DynamoDB::Model::AttributeValue
attribute_value(concepts::timestamp auto const timestamp) {
  std::array<char, 32> buffer;
  std::size_t const n = std::snprintf(buffer.data(), buffer.size(), "%zd",
                                      timestamp.time_since_epoch().count());
  return Aws::DynamoDB::Model::AttributeValue{}.SetN(
      Aws::String{buffer.data(), n});
}

inline Aws::DynamoDB::Model::AttributeValue attribute_value(
    Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> subobject) {
  Aws::DynamoDB::Model::AttributeValue result;
  for (auto &[key, value] : subobject) {
    result.AddMEntry(key,
                     std::make_shared<Aws::DynamoDB::Model::AttributeValue>(
                         std::move(value)));
  }
  return result;
}

namespace safe_get_item_value_details_ {

struct fn final {
  template <concepts::dictionary Dictionary,
            typename Projection = std::identity>
  constexpr auto operator()(Dictionary &item, key_t<Dictionary> const &key,
                            Projection project = {}) const noexcept(false)
      -> std::invoke_result_t<Projection, decltype(item.find(key)->second)> {
    auto const result_iter = item.find(key);
    if (std::ranges::end(item) == result_iter) {
      throw_invalid_argument(key);
    } else {
      return std::invoke(project, result_iter->second);
    }
  }

  template <concepts::lookup_dictionary Dictionary,
            typename Projection = std::identity>
  constexpr auto operator()(Dictionary const &item,
                            key_t<Dictionary> const &key,
                            Projection project = {}) const noexcept(false)
      -> std::invoke_result_t<Projection, decltype(item.find(key)->second)> {
    auto const result_iter = item.find(key);
    if (std::end(item) == result_iter) {
      throw_invalid_argument(key);
    } else {
      return std::invoke(project, result_iter->second);
    }
  }

private:
  [[noreturn]] constexpr void throw_invalid_argument(auto const &key) const
      noexcept(false) {
    std::ostringstream message;
    message << "Could not find '" << key << "'";
    throw std::invalid_argument{message.str()};
  }
};
} // namespace safe_get_item_value_details_

inline constexpr safe_get_item_value_details_::fn safe_get_item_value = {};

template <typename T>
T get_value_from_item(
    Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> const &item,
    Aws::String const &key);

template <>
inline Aws::String get_value_from_item<Aws::String>(
    Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> const &item,
    Aws::String const &key) noexcept(false) {
  auto const attribute_iter = item.find(key);
  if (std::end(item) == attribute_iter) {
    throw std::invalid_argument{"Could not find attribute for '" + key + "'"};
  } else {
    return attribute_iter->second.GetS();
  }
}

template <std::integral I>
inline I get_value_from_item(
    Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> const &item,
    Aws::String const &key) noexcept(false) {
  auto const str_value = safe_get_item_value(
      item, key, &Aws::DynamoDB::Model::AttributeValue::GetN);
  I return_value;
  auto const result = std::from_chars(
      str_value.data(), str_value.data() + str_value.size(), return_value);
  auto const status = std::make_error_code(result.ec);
  if (status) {
    throw std::system_error{status};
  } else {
    return return_value;
  }
}

template <concepts::timestamp Timestamp>
inline Timestamp get_value_from_item(
    Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> const &item,
    Aws::String const &key) noexcept(false) {
  return Timestamp{typename Timestamp::duration{
      get_value_from_item<typename Timestamp::duration::rep>(item, key)}};
}

} // namespace skizzay::cddd::dynamodb
