#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace skizzay::cddd::dynamodb {

struct ttl_attributes {
  std::string name;
  std::chrono::seconds value;
};

struct event_log_config {
  explicit event_log_config(std::string key_name, std::string version_name,
                            std::string timestamp_name, std::string type_name,
                            std::string table_name)
      : key_name_{std::move(key_name)}, version_name_{std::move(version_name)},
        max_version_name_{version_name_ + "_max_"}, timestamp_name_{std::move(
                                                        timestamp_name)},
        type_name_{std::move(type_name)}, table_name_{std::move(table_name)},
        ttl_attributes_{std::nullopt} {}

  explicit event_log_config(std::string key_name, std::string version_name,
                            std::string timestamp_name, std::string type_name,
                            std::string table_name, std::string ttl_name,
                            std::chrono::seconds ttl_duration)
      : event_log_config{std::move(key_name), std::move(version_name),
                         std::move(timestamp_name), std::move(type_name),
                         std::move(table_name)} {
    ttl_attributes_ = ttl_attributes{std::move(ttl_name), ttl_duration};
  }

  std::string const &key_name() const noexcept { return key_name_; }
  std::string const &version_name() const noexcept { return version_name_; }
  std::string const &max_version_name() const noexcept {
    return max_version_name_;
  }
  std::string const &timestamp_name() const noexcept { return timestamp_name_; }
  std::string const &type_name() const noexcept { return type_name_; }
  std::string const &table_name() const noexcept { return table_name_; }
  std::optional<ttl_attributes> const &ttl() const noexcept {
    return ttl_attributes_;
  }

private:
  std::string key_name_;
  std::string version_name_;
  std::string max_version_name_;
  std::string timestamp_name_;
  std::string type_name_;
  std::string table_name_;
  std::optional<ttl_attributes> ttl_attributes_;
};

} // namespace skizzay::cddd::dynamodb
