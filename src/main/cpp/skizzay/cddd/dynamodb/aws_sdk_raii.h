#pragma once

#include <aws/core/Aws.h>

namespace skizzay::cddd::dynamodb {
struct aws_sdk_raii final {
  aws_sdk_raii(Aws::SDKOptions const &options) : options_{options} {
    Aws::InitAPI(options_);
  }

  ~aws_sdk_raii() { Aws::ShutdownAPI(options_); }

private:
  Aws::SDKOptions const options_;
};
} // namespace skizzay::cddd::dynamodb
