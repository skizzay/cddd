#pragma once

#include <aws/dynamodb/model/AttributeValue.h>
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
  // TODO: narrow_cast
  return attribute_value(static_cast<int>(value));
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
