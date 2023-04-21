#pragma once

#include "skizzay/cddd/commit_failed.h"
#include "skizzay/cddd/dynamodb/dynamodb_attribute_value.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_log_config.h"
#include "skizzay/cddd/dynamodb/dynamodb_operation_failed_error.h"
#include "skizzay/cddd/dynamodb/dynamodb_ttl.h"
#include "skizzay/cddd/event_stream_buffer.h"
#include "skizzay/cddd/identifier.h"
#include "skizzay/cddd/narrow_cast.h"
#include "skizzay/cddd/nullable.h"
#include "skizzay/cddd/optimistic_concurrency_collision.h"
#include "skizzay/cddd/version.h"
#include "skizzay/cddd/views.h"

#include <aws/dynamodb/DynamoDBErrors.h>
#include <aws/dynamodb/model/Delete.h>
#include <aws/dynamodb/model/Put.h>
#include <aws/dynamodb/model/TransactWriteItem.h>
#include <aws/dynamodb/model/TransactWriteItemsRequest.h>
#include <optional>
#include <ranges>
#include <string>

namespace skizzay::cddd::dynamodb {
namespace event_stream_details_ {
inline std::string const version_record_message_type = "version_";
inline constexpr std::size_t dynamodb_batch_size = 25;

constexpr void add_standard_field(standard_field const &field,
                                  Aws::DynamoDB::Model::Put &put,
                                  auto &&value) {
  put.AddItem(field.name,
              attribute_value(std::forward<decltype(value)>(value)));
}

constexpr void
add_standard_field(std::optional<time_to_live_field> const &field,
                   Aws::DynamoDB::Model::Put &put,
                   concepts::timestamp auto const &timestamp) {
  if (not skizzay::cddd::is_null(field)) {
    add_standard_field(*field, put,
                       // TODO: Cast this TTL value back to UTC time
                       attribute_value(timestamp + field->duration));
  }
}

inline constexpr auto as_transact_write_item = [](Aws::DynamoDB::Model::Put put)
    -> Aws::DynamoDB::Model::TransactWriteItem {
  return Aws::DynamoDB::Model::TransactWriteItem{}.WithPut(std::move(put));
};

template <typename T>
using commit_error = operation_failed_error<commit_failed, T>;

[[noreturn]] void
throw_exception(auto const &error,
                concepts::version auto const expected_version) noexcept(false) {
  switch (error.GetErrorType()) {
  case Aws::DynamoDB::DynamoDBErrors::CONDITIONAL_CHECK_FAILED:
  case Aws::DynamoDB::DynamoDBErrors::DUPLICATE_ITEM:
  case Aws::DynamoDB::DynamoDBErrors::TRANSACTION_CONFLICT:
    throw optimistic_concurrency_collision{error.GetMessage(),
                                           expected_version};

  default:
    throw commit_error{error};
  }
}

// template <typename Store> struct impl {
//   template <concepts::identifier Id, concepts::version Version>
//   constexpr void
//   commit_events(Id id_value, Version const expected_version,
//                 event_stream_buffer_t<Store> event_stream_buffer) {
//     if (not std::ranges::empty(event_stream_buffer)) {
//       concepts::timestamp auto const timestamp =
//           skizzay::cddd::now(store_->clock());
//       for (auto &&[i, element] : views::enumerate(event_stream_buffer)) {
//         set_item_value(element, store_->hash_key_name(), id_value);
//         set_item_value(element, store_->sort_key_name(),
//                        expected_version + narrow_cast<Version>(i + 1));
//         set_item_value(element, store_->timestamp_name(), timestamp);
//       }
//       commit_buffered_events(std::move(id_value),
//                              std::move(event_stream_buffer),
//                              expected_version);
//     }
//   }

// private:
//   constexpr void commit_buffered_events(auto &&id_value,
//                                         event_stream_buffer_t<Store>
//                                         &&buffer, auto const
//                                         expected_version) {}
// };

// template <std::unsigned_integral Version>
// Version parse_version(std::string const &version_string) {
//   Version parsed_value = std::numeric_limits<Version>::max();
//   auto const parse_result = std::from_chars(
//       version_string.data(), version_string.data() + version_string.size(),
//       parsed_value);
//   std::error_code const result_code = std::make_error_code(parse_result.ec);
//   if (result_code || (std::numeric_limits<Version>::max() == parsed_value)) {
//     throw version_validation_error{
//         result_code, "Cannot parse valid version from : " + version_string};
//   } else {
//     return parsed_value;
//   }
// }

// template <concepts::clock Clock, concepts::domain_event... DomainEvents>
// struct impl : event_stream_base<impl<Clock, DomainEvents...>, Clock,
//                                 Aws::DynamoDB::Model::Put, DomainEvents...> {
//   using base_type =
//       event_stream_base<impl<Clock, DomainEvents...>, Clock,
//                         Aws::DynamoDB::Model::Put, DomainEvents...>;
//   using typename base_type::buffer_type;
//   using typename base_type::element_type;
//   using typename base_type::id_type;
//   using typename base_type::timestamp_type;
//   using typename base_type::version_type;
//   using item_type = Aws::Map<Aws::String,
//   Aws::DynamoDB::Model::AttributeValue>;

//   template
//   <concepts::factory<Aws::DynamoDB::Model::TransactWriteItemsRequest>
//                 CommitRequestFactory = default_factory<
//                     Aws::DynamoDB::Model::TransactWriteItemsRequest>>
//   impl(id_type id, serializer<DomainEvents...> &serializer,
//        event_log_config const &config, Aws::DynamoDB::DynamoDBClient &client,
//        Clock clock, CommitRequestFactory &&get_request = {})
//       : base_type{std::move(clock)}, id_{id},
//         serializer_{serializer}, config_{config}, client_{client},
//         get_request_{std::forward<decltype(get_request)>(get_request)} {}

//   id_type id() const noexcept { return id_; }

//   version_type version() const {
//     auto extract_version_from_outcome = [this](auto &version_record) {
//       auto const max_version_iterator =
//           version_record.find(config_.max_version_name());
//       return (std::end(version_record) == max_version_iterator)
//                  ? 0
//                  : parse_version<version_type>(
//                        max_version_iterator->second.GetN());
//     };
//     auto outcome = client_.GetItem(
//         Aws::DynamoDB::Model::GetItemRequest{}
//             .WithTableName(config_.table_name())
//             .WithKey(key())
//             .WithProjectionExpression(config_.max_version_name())
//             .WithConsistentRead(true));
//     if (outcome.IsSuccess()) {
//       return extract_version_from_outcome(outcome.GetResult().GetItem());
//     } else {
//       throw operation_failed_error<std::runtime_error,
//                                    Aws::DynamoDB::DynamoDBError>{
//           outcome.GetError()};
//     }
//   }

//   void commit_buffered_events(buffer_type &&buffer, timestamp_type timestamp,
//                               version_type expected_version) {
//     auto commit_batch =
//         [this, expected_version](std::ranges::input_range auto batch) mutable
//         {
//           auto request = get_request_().WithTransactItems(
//               Aws::Vector<Aws::DynamoDB::Model::TransactWriteItem>{
//                   std::ranges::begin(batch), std::ranges::end(batch)});
//           auto const outcome = client_.TransactWriteItems(request);
//           if (!outcome.IsSuccess()) {
//             throw_exception(outcome.GetError(), expected_version);
//           }
//         };
//     auto as_transact_write_item = [](auto &put) {
//       return
//       Aws::DynamoDB::Model::TransactWriteItem{}.WithPut(std::move(put));
//     };
//     auto items_to_commit = views::concat(
//         views::single(get_version_write_item(timestamp, expected_version)),
//         buffer | views::transform(as_transact_write_item));
//     std::ranges::for_each(items_to_commit |
//     views::chunk(dynamodb_batch_size),
//                           commit_batch);
//   }

//   template <concepts::domain_event DomainEvent>
//   Aws::DynamoDB::Model::Put make_buffer_element(DomainEvent &&domain_event) {
//     deser_details_::serializer_interface<std::remove_cvref_t<DomainEvent>>
//         &serializer = static_cast<deser_details_::serializer_interface<
//             std::remove_cvref_t<DomainEvent>> &>(serializer_);
//     auto result = serializer.serialize(std::move(domain_event));
//     initialize(result, serializer.message_type(event_type<DomainEvent>{}));
//     return result;
//   }

//   void populate_commit_info(concepts::timestamp auto timestamp,
//                             version_type event_version,
//                             Aws::DynamoDB::Model::Put &put) {
//     put.AddItem(config_.timestamp_name(), attribute_value(timestamp))
//         .AddItem(config_.version_name(), attribute_value(event_version));
//     if (config_.ttl().has_value()) {
//       auto const &ttl_attributes = config_.ttl().value();
//       put.AddItem(
//           ttl_attributes.name,
//           attribute_value(
//               std::chrono::time_point_cast<std::chrono::seconds>(timestamp) +
//               ttl_attributes.value));
//     }
//   }

// private:
//   Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> key() {
//     return {{this->config_.key_name(), attribute_value(id())},
//             {this->config_.version_name(), attribute_value(0)}};
//   }

//   void initialize(Aws::DynamoDB::Model::Put &put, std::string_view type) {
//     put.WithTableName(config_.table_name())
//         .AddItem(config_.key_name(), attribute_value(id()))
//         .AddItem(config_.type_name(), attribute_value(type))
//         .WithConditionExpression("attribute_not_exists(" + config_.key_name()
//         +
//                                  ")");
//   }

//   Aws::DynamoDB::Model::Put
//   get_starting_version_item(concepts::timestamp auto timestamp) {
//     auto result = Aws::DynamoDB::Model::Put{}.AddItem(
//         config_.max_version_name(), attribute_value(std::size(items_)));
//     initialize(result, version_record_message_type);
//     populate_commit_info(timestamp, 0, result);
//     return result;
//   }

//   Aws::DynamoDB::Model::Update
//   get_update_version_item(concepts::timestamp auto timestamp,
//                           version_type expected_version) {
//     auto const expression_attribute_values = [=, this]() {
//       item_type result{{":inc", attribute_value(std::size(items_))},
//                        {":ver", attribute_value(expected_version)},
//                        {":ts", attribute_value(timestamp)}};
//       if (config_.ttl()) {
//         // TODO: clock_cast is missing from GCC 11
//         auto expiration = std::chrono::time_point_cast<std::chrono::seconds>(
//             timestamp + config_.ttl()->value);
//         result.emplace(":ttl",
//                        attribute_value(std::chrono::sys_seconds{expiration}));
//       }
//       return result;
//     };
//     auto const expression_attribute_names = [this]() {
//       Aws::Map<Aws::String, Aws::String> result{
//           {"#ver", config_.max_version_name()},
//           {"#ts", config_.timestamp_name()}};
//       if (config_.ttl()) {
//         result.emplace("#ttl", config_.ttl()->name);
//       }
//       return result;
//     };
//     auto const update_expression = [this]() {
//       return config_.ttl().has_value()
//                  ? "set #ver = #ver + :inc, #ts = :ts, #ttl = :ttl"
//                  : "set #ver = #ver + :inc, #ts = :ts";
//     };
//     return Aws::DynamoDB::Model::Update{}
//         .WithTableName(config_.table_name())
//         .WithKey(key())
//         .WithUpdateExpression(update_expression())
//         .WithConditionExpression("#ver = :ver")
//         .WithExpressionAttributeNames(expression_attribute_names())
//         .WithExpressionAttributeValues(expression_attribute_values());
//   }

//   Aws::DynamoDB::Model::TransactWriteItem
//   get_version_write_item(concepts::timestamp auto timestamp,
//                          version_type expected_version) {
//     using Aws::DynamoDB::Model::TransactWriteItem;
//     if (0 == expected_version) {
//       return
//       TransactWriteItem{}.WithPut(get_starting_version_item(timestamp));
//     } else {
//       return TransactWriteItem{}.WithUpdate(
//           get_update_version_item(timestamp, expected_version));
//     }
//   }

//   [[noreturn]] void
//   throw_exception(auto const &error,
//                   version_type const expected_version) noexcept(false) {
//     switch (error.GetErrorType()) {
//     case Aws::DynamoDB::DynamoDBErrors::CONDITIONAL_CHECK_FAILED:
//     case Aws::DynamoDB::DynamoDBErrors::DUPLICATE_ITEM:
//     case Aws::DynamoDB::DynamoDBErrors::TRANSACTION_CONFLICT:
//       throw optimistic_concurrency_collision{error.GetMessage(),
//                                              expected_version};

//     default:
//       throw commit_error{error};
//     }
//   }

//   std::remove_cvref_t<id_type> id_;
//   serializer<DomainEvents...> &serializer_;
//   event_log_config const &config_;
//   Aws::DynamoDB::DynamoDBClient &client_;
//   [[no_unique_address]] Clock clock_;
//   std::function<Aws::DynamoDB::Model::TransactWriteItemsRequest()>
//   get_request_; std::deque<Aws::DynamoDB::Model::Put> items_;
// };

template <typename T>
concept put_range = std::ranges::range<T> &&
    std::same_as<Aws::DynamoDB::Model::Put, std::ranges::range_value_t<T>>;

template <typename Store> struct impl {
  friend Store;

  constexpr void
  commit_events(concepts::identifier auto id_value,
                concepts::version auto const expected_version,
                concepts::event_stream_buffer auto event_stream_buffer) {
    if (not std::ranges::empty(event_stream_buffer)) {
      concepts::timestamp auto const timestamp =
          skizzay::cddd::now(store_->clock());
      auto version_put = get_starting_version(
          expected_version, std::ranges::size(event_stream_buffer), timestamp,
          id_value);
      auto puts_with_commit_info =
          views::enumerate(event_stream_buffer) |
          views::transform(
              [&](std::tuple<std::size_t, Aws::DynamoDB::Model::Put &>
                      index_and_put) {
                add_commit_info(id_value, expected_version, timestamp,
                                std::get<0>(index_and_put),
                                std::get<1>(index_and_put));
                return std::get<1>(index_and_put);
              });
      auto puts =
          views::concat(views::single(version_put), puts_with_commit_info);
      commit_puts_to_dynamodb(puts, expected_version);
    }
  }

  constexpr void rollback_to(concepts::identifier auto const &,
                             concepts::version auto const) {}

private:
  constexpr impl(Store &store) noexcept : store_{&store} {}

  constexpr Aws::DynamoDB::Model::Put &add_commit_info(
      concepts::identifier auto const &id_value, std::size_t const save_version,
      concepts::timestamp auto const timestamp,
      std::string const &condition_expression, Aws::DynamoDB::Model::Put &put) {
    add_standard_field(store_->config().hash_key(), put, id_value);
    add_standard_field(store_->config().sort_key(), put, save_version);
    add_standard_field(store_->config().timestamp_field(), put, timestamp);
    add_standard_field(store_->config().ttl_field(), put, timestamp);
    return put.WithConditionExpression(condition_expression)
        .WithTableName(store_->config().table().get());
  }

  constexpr Aws::DynamoDB::Model::Put &
  add_commit_info(concepts::identifier auto const &id_value,
                  concepts::version auto const expected_version,
                  concepts::timestamp auto const timestamp,
                  std::size_t const index, Aws::DynamoDB::Model::Put &put) {
    return add_commit_info(id_value, expected_version + index + 1, timestamp,
                           store_->new_condition_expression(), put);
  }

  constexpr Aws::DynamoDB::Model::Put
  get_starting_version(concepts::version auto const expected_version,
                       std::size_t const num_elements,
                       concepts::timestamp auto const timestamp,
                       concepts::identifier auto id_value) {
    std::string const &condition_expression =
        (0 == expected_version) ? store_->new_condition_expression()
                                : store_->update_condition_expression();
    Aws::DynamoDB::Model::Put put;
    put.AddExpressionAttributeValues(
        store_->config().max_version_field().value_expression,
        attribute_value(expected_version + num_elements));
    add_commit_info(id_value, std::size_t{0}, timestamp, condition_expression,
                    put);
    add_standard_field(store_->config().max_version_field(), put,
                       expected_version + num_elements);
    return put;
  }

  void commit_puts_to_dynamodb(put_range auto puts,
                               concepts::version auto const expected_version) {
    auto transactions = puts | views::transform(as_transact_write_item) |
                        views::chunk(dynamodb_batch_size);
    std::ranges::for_each(transactions, [this, expected_version](auto items) {
      commit_transaction(transact_write_items_request(items), expected_version);
    });
  }

  void commit_transaction(auto &&request,
                          concepts::version auto const expected_version) {
    auto const outcome = store_->client().commit(std::move(request));
    if (!outcome.IsSuccess()) {
      throw_exception(outcome.GetError(), expected_version);
    }
  }

  template <std::ranges::input_range Items>
  requires std::same_as<Aws::DynamoDB::Model::TransactWriteItem,
                        std::ranges::range_value_t<Items>>
      Aws::DynamoDB::Model::TransactWriteItemsRequest
      transact_write_items_request(Items &items) {
    Aws::Vector<Aws::DynamoDB::Model::TransactWriteItem> transaction;
    transaction.reserve(dynamodb_batch_size);
    std::ranges::move(std::move(items), std::back_inserter(transaction));
    Aws::DynamoDB::Model::TransactWriteItemsRequest request;
    request.SetTransactItems(std::move(transaction));
    return request;
  }

  Store *store_;
};
} // namespace event_stream_details_

template <typename Store>
using event_stream = event_stream_details_::impl<Store>;

} // namespace skizzay::cddd::dynamodb
