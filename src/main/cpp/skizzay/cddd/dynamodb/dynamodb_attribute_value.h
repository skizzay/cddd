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
using record = Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>;

namespace attribute_value_details_ {
struct attibute_value_fn final {
  inline Aws::DynamoDB::Model::AttributeValue
  operator()(bool const value) const {
    return Aws::DynamoDB::Model::AttributeValue{}.SetBool(value);
  }

  inline Aws::DynamoDB::Model::AttributeValue
  operator()(std::integral auto const value) const {
    return Aws::DynamoDB::Model::AttributeValue{}.SetN(std::to_string(value));
  }

  inline Aws::DynamoDB::Model::AttributeValue
  operator()(std::string_view const value) const {
    return Aws::DynamoDB::Model::AttributeValue{}.SetS(
        Aws::String{value.data(), value.size()});
  }

  inline Aws::DynamoDB::Model::AttributeValue
  operator()(std::floating_point auto const value) const {
    return Aws::DynamoDB::Model::AttributeValue{}.SetN(value);
  }

  template <typename T>
  requires std::is_constructible_v < Aws::DynamoDB::Model::AttributeValue,
      std::remove_reference_t<T>
  const & > inline Aws::DynamoDB::Model::AttributeValue
            operator()(T const &value) const {
    return Aws::DynamoDB::Model::AttributeValue{value};
  }

  inline Aws::DynamoDB::Model::AttributeValue
  operator()(concepts::timestamp auto const timestamp) const {
    std::array<char, 32> buffer;
    std::size_t const n = std::snprintf(buffer.data(), buffer.size(), "%zd",
                                        timestamp.time_since_epoch().count());
    return Aws::DynamoDB::Model::AttributeValue{}.SetN(
        Aws::String{buffer.data(), n});
  }

  inline Aws::DynamoDB::Model::AttributeValue
  operator()(record subobject) const {
    Aws::DynamoDB::Model::AttributeValue result;
    for (auto &[key, value] : std::move(subobject)) {
      result.AddMEntry(std::move(key),
                       std::make_shared<Aws::DynamoDB::Model::AttributeValue>(
                           std::move(value)));
    }
    return result;
  }
};
} // namespace attribute_value_details_

inline namespace attribute_value_fn_ {
inline constexpr attribute_value_details_::attibute_value_fn attribute_value =
    {};
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

inline namespace safe_get_item_value_fn_ {
inline constexpr safe_get_item_value_details_::fn safe_get_item_value = {};
}

namespace get_item_or_default_value_details_ {
struct get_item_or_default_value_fn final {
  void operator()(record const &item, Aws::String const &key,
                  Aws::String &value) const noexcept {
    auto const attribute_iter = item.find(key);
    if (std::end(item) != attribute_iter) {
      value = attribute_iter->second.GetS();
    }
  }

  void operator()(record const &item, Aws::String const &key,
                  std::integral auto &value) const noexcept(false) {
    auto const attribute_iter = item.find(key);
    if (std::end(item) != attribute_iter) {
      auto const str_value = attribute_iter->second.GetN();
      auto const result = std::from_chars(
          str_value.data(), str_value.data() + str_value.size(), value);
      auto const status = std::make_error_code(result.ec);
      if (status) {
        throw std::system_error{status};
      }
    }
  }

  void operator()(record const &item, Aws::String const &key,
                  concepts::timestamp auto &value) const noexcept {
    using Timestamp = std::remove_reference_t<decltype(value)>;
    typename Timestamp::duration::rep duration_value{};
    (*this)(item, key, duration_value);
    value = Timestamp{typename Timestamp::duration{duration_value}};
  }

  void operator()(record const &item, Aws::String const &key,
                  bool &value) const noexcept {
    auto const attribute_iter = item.find(key);
    if (std::end(item) != attribute_iter) {
      value = attribute_iter->second.GetBool();
    }
  }

  template <std::default_initializable T>
  T from(record const &item, Aws::String const &key) const noexcept(
      std::is_nothrow_invocable_v<get_item_or_default_value_fn const,
                                  record const &, Aws::String const &, T &>) {
    T result{};
    (*this)(item, key, result);
    return result;
  }
};
} // namespace get_item_or_default_value_details_

inline namespace get_item_or_default_value_fn_ {
inline constexpr get_item_or_default_value_details_::
    get_item_or_default_value_fn get_item_or_default_value = {};
}

} // namespace skizzay::cddd::dynamodb
