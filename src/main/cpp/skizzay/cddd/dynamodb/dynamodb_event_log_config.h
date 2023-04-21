#pragma once

#include "skizzay/cddd/dynamodb/dynamodb_clock.h"
#include "skizzay/cddd/value.h"

#include <optional>
#include <string>

namespace skizzay::cddd::dynamodb {

using table_name = value<struct table_name_tag, std::string>;

namespace event_log_config_details_ {
inline constexpr auto throw_invalid_value = [](std::string_view field_name,
                                               std::string_view field_type,
                                               auto const &field_setting) {
  std::ostringstream message;
  message << field_name << " - Invalid " << field_type
          << " provided:" << field_setting;
  throw std::invalid_argument{std::move(message).str()};
};

struct standard_field {
  std::string name;
  std::string name_expression;
  std::string value_expression;

  void validate(std::string_view field_name) const {
    if (std::empty(name) || name.starts_with(':') || name.starts_with('#')) {
      throw_invalid_value(field_name, "name", name);
    } else if (std::empty(name_expression) ||
               !name_expression.starts_with('#')) {
      throw_invalid_value(field_name, "name expression", name_expression);
    } else if (std::empty(value_expression) ||
               !value_expression.starts_with(':')) {
      throw_invalid_value(field_name, "value expression", value_expression);
    }
  }
};

struct time_to_live_field : standard_field {
  std::chrono::seconds duration;

  void validate() const {
    constexpr std::string_view field_name = "Time-to-live";
    this->standard_field::validate(field_name);
    if (duration <= std::chrono::seconds{}) {
      throw_invalid_value(field_name, "duration", duration);
    }
  }
};

struct impl {
  constexpr standard_field const &hash_key() const noexcept {
    return hash_key_;
  }

  constexpr standard_field const &sort_key() const noexcept {
    return sort_key_;
  }

  constexpr standard_field const &timestamp_field() const noexcept {
    return timestamp_field_;
  }

  constexpr std::optional<time_to_live_field> const &
  ttl_field() const noexcept {
    return time_to_live_;
  }

  constexpr standard_field const &max_version_field() const noexcept {
    return max_version_field_;
  }

  constexpr table_name const &table() const noexcept { return table_name_; }

  impl &with_hash_key(std::string name, std::string name_expression,
                      std::string value_expression) {
    standard_field new_value{std::move(name), std::move(name_expression),
                             std::move(value_expression)};
    new_value.validate("hash key");
    hash_key_ = std::move(new_value);
    return *this;
  }

  impl &with_sort_key(std::string name, std::string name_expression,
                      std::string value_expression) {
    standard_field new_value{std::move(name), std::move(name_expression),
                             std::move(value_expression)};
    new_value.validate("sort key");
    sort_key_ = std::move(new_value);
    return *this;
  }

  impl &with_timestamp_field(std::string name, std::string name_expression,
                             std::string value_expression) {
    standard_field new_value{std::move(name), std::move(name_expression),
                             std::move(value_expression)};
    new_value.validate("timestamp");
    timestamp_field_ = std::move(new_value);
    return *this;
  }

  constexpr impl &without_ttl() {
    time_to_live_ = std::nullopt;
    return *this;
  }

  impl &with_ttl(std::string name, std::string name_expression,
                 std::string value_expression, std::chrono::seconds duration) {
    time_to_live_field new_value{standard_field{std::move(name),
                                                std::move(name_expression),
                                                std::move(value_expression)},
                                 std::move(duration)};
    new_value.validate();
    time_to_live_.emplace(std::move(new_value));
    return *this;
  }

  impl &with_ttl(std::chrono::seconds duration) {
    return with_ttl("ttl", "#ttl", ":ttl", std::move(duration));
  }

  impl &with_max_version_field(std::string name, std::string name_expression,
                               std::string value_expression) {
    standard_field new_value{std::move(name), std::move(name_expression),
                             std::move(value_expression)};
    new_value.validate("max version");
    max_version_field_ = std::move(new_value);
    return *this;
  }

  impl &with_table(table_name value) {
    if (std::empty(value.get())) {
      throw std::invalid_argument{"Empty table name provided"};
    } else {
      table_name_ = std::move(value);
      return *this;
    }
  }

private:
  table_name table_name_;
  standard_field hash_key_{"hk", "#hk", ":hk"};
  standard_field sort_key_{"sk", "#sk", ":sk"};
  standard_field timestamp_field_{"t", "#t", ":t"};
  std::optional<time_to_live_field> time_to_live_{std::nullopt};
  standard_field max_version_field_{"v", "#v", ":v"};
};
} // namespace event_log_config_details_

using standard_field = event_log_config_details_::standard_field;
using time_to_live_field = event_log_config_details_::time_to_live_field;
using event_log_config = event_log_config_details_::impl;

} // namespace skizzay::cddd::dynamodb
