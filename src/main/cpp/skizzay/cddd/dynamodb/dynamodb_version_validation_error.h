#pragma once

#include <stdexcept>
#include <system_error>

namespace skizzay::cddd::dynamodb {
struct version_validation_error : std::runtime_error {
  version_validation_error(std::error_code ec, std::string_view message)
      : std::runtime_error{message.data()}, ec{ec} {}

  version_validation_error(std::string_view message)
      : version_validation_error{{}, message} {}

  std::error_code ec;
};
} // namespace skizzay::cddd::dynamodb
