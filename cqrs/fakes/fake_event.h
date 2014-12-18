#ifndef CDDD_CQRS_FAKE_EVENT_H__
#define CDDD_CQRS_FAKE_EVENT_H__

#include "cqrs/domain_event.h"

namespace cddd {
namespace cqrs {

struct fake_event final : domain_event {
   virtual ~fake_event() final = default;

   enum { cache_line_size = 64 };
   double d;
   int i;
   char c[cache_line_size-(alignof(d) + alignof(i))];

   virtual std::type_index type() const final override {
      return typeid(*this);
   }
   virtual std::size_t version() const final override {
      return static_cast<std::size_t>(i);
   }
};

}
}

#endif
