#pragma once

#include "skizzay/cddd/history_load_failed.h"
#include <stdexcept>
#include <system_error>

namespace skizzay::cddd::dynamodb {
struct version_validation_error : event_deserialization_failed {
  version_validation_error(std::error_code ec, std::string const &message)
      : event_deserialization_failed{message}, ec{ec} {}

  std::error_code ec;
};
} // namespace skizzay::cddd::dynamodb
