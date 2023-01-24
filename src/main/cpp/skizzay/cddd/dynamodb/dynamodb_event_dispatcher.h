#pragma once

#include "skizzay/cddd/aggregate_root.h"
#include "skizzay/cddd/domain_event.h"
#include "skizzay/cddd/dynamodb/dynamodb_deser.h"
#include "skizzay/cddd/dynamodb/dynamodb_event_log_config.h"
#include "skizzay/cddd/history_load_failed.h"
#include <concepts>
#include <functional>
#include <initializer_list>
#include <unordered_map>

namespace skizzay::cddd::dynamodb {

namespace event_dispatcher_details_ {

template <typename T>
concept translator = std::invocable < T,
        Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>
const & >
    &&concepts::domain_event<std::invoke_result_t<
        T,
        Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> const &>>;

template <translator Translator>
using domain_event_result_t = std::remove_cvref_t<std::invoke_result_t<
    Translator,
    Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> const &>>;

template <typename T, typename DomainEvents>
concept translator_for_one_of =
    translator<T> && concepts::domain_event_sequence<DomainEvents> &&
    DomainEvents::template contains<domain_event_result_t<T>>;
} // namespace event_dispatcher_details_

template <concepts::domain_event_sequence DomainEvents>
struct event_dispatcher {
  using item_type = Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>;
  using handler_type =
      std::function<void(item_type const &, event_visitor<DomainEvents> &)>;

  event_dispatcher(event_log_config const &config) noexcept
      : config_{config}, handlers_{} {}

  void dispatch(item_type const &item, event_visitor<DomainEvents> &visitor) {
    try {
      std::string const type =
          safe_get_item_value(item, config_.type_name(),
                              &Aws::DynamoDB::Model::AttributeValue::GetS);
      auto const handler_iter = handlers_.find(type);
      if (std::end(handlers_) == handler_iter) {
        throw std::invalid_argument{"Could not find handler for '" + type +
                                    "'"};
      } else {
        handler_iter->second(item, visitor);
      }
    } catch (...) {
      std::throw_with_nested(event_deserialization_failed{
          "Event dispatcher failed to dispatch event to handler."});
    }
  }

  void register_translator(
      std::string event_type_name,
      event_dispatcher_details_::translator_for_one_of<DomainEvents> auto
          translator) {
    if (handlers_.contains(event_type_name)) {
      throw std::logic_error{"Handler for event '" + event_type_name +
                             "' already registered"};
    } else {
      auto handler = [translator = std::move(translator)](
                         item_type const &item,
                         event_visitor<DomainEvents> &v) {
        static_cast<event_visitor_interface<
            event_dispatcher_details_::domain_event_result_t<
                decltype(translator)>> &>(v)
            .visit(std::invoke(translator, item));
      };
      handlers_.emplace(std::move(event_type_name), std::move(handler));
    }
  }

private:
  event_log_config const &config_;
  std::unordered_map<std::string, handler_type> handlers_;
};

template <event_dispatcher_details_::translator... Translators>
event_dispatcher(event_log_config const &,
                 std::pair<std::string_view, Translators>...)
    -> event_dispatcher<
        event_dispatcher_details_::domain_event_result_t<Translators>...>;
} // namespace skizzay::cddd::dynamodb
