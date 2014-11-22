#ifndef CDDD_CQRS_FAKE_DISPATCHER_H__
#define CDDD_CQRS_FAKE_DISPATCHER_H__

#include "cqrs/event_dispatcher.h"
#include <gmock/gmock.h>

namespace cddd {
namespace cqrs {

class fake_dispatcher : public domain_event_dispatcher {
public:
   class spy_type {
   public:
      MOCK_METHOD2(add_handler, void(std::type_index, domain_event_handler));
      MOCK_METHOD1(dispatch, void(const domain_event &));
   };

   explicit inline fake_dispatcher(std::shared_ptr<spy_type> spy_=std::make_shared<spy_type>()) :
      domain_event_dispatcher(),
      spy(spy_)
   {
   }
   fake_dispatcher(fake_dispatcher &&) = default;
   virtual ~fake_dispatcher() = default;

   template<class Evt, class Fun>
   inline void add_handler(Fun &&f) {
      domain_event_handler closure{[f=std::forward<Fun>(f)](const domain_event &e) {
         f(static_cast<const details_::domain_event_wrapper<Evt>&>(e).evt);
      }};
      spy->add_handler(typeid(Evt), std::move(closure));
   }

   inline void dispatch(const domain_event &e) {
      spy->dispatch(e);
   }

   std::shared_ptr<spy_type> spy;
};

}
}

#endif
