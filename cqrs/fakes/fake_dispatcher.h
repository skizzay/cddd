#ifndef CDDD_CQRS_FAKE_DISPATCHER_H__
#define CDDD_CQRS_FAKE_DISPATCHER_H__

#include "messaging/traits.h"
#include <gmock/gmock.h>

namespace cddd {
namespace cqrs {

class fake_dispatcher {
public:
   typedef std::function<void(const domain_event &)> domain_event_handler;

   class spy_type {
   public:
      MOCK_METHOD1(add_message_handler, void(domain_event_handler));
      MOCK_METHOD1(dispatch_message, void(const domain_event &));
   };

   explicit inline fake_dispatcher(std::shared_ptr<spy_type> spy_=std::make_shared<spy_type>()) :
      spy(spy_)
   {
   }
   fake_dispatcher(fake_dispatcher &&) = default;
   virtual ~fake_dispatcher() = default;

   template<class Fun, class Filter>
   inline void add_message_handler(Fun, Filter) {
      spy->add_message_handler(domain_event_handler{});
   }

   inline void dispatch_message(const domain_event &e) {
      spy->dispatch_message(e);
   }

   std::shared_ptr<spy_type> spy;
};

}
}

#endif
