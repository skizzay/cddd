#pragma once

#include "skizzay/cddd/identifier.h"
#include <concepts>
#include <aws/dynamodb/model/AttributeValue.h>

namespace skizzay::cddd::dynamodb {
namespace identifier_details_ {
template <typename> struct set_impl;

template<std::integral Id>
struct set_impl<Id> {
   static void set(auto &t, AttributeValue const &value) {
      
   }
};
} // namespace identifier_details_

} // namespace skizzay::cddd::dynamodb

namespace Aws::DynamoDB::Model {
void set_id(auto &t, AttributeValue const &value) {}
} // namespace Aws::DynamoDB::Model
