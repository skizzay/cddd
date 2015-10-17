#ifndef CDDD_CQRS_DOMAIN_EVENT_H__
#define CDDD_CQRS_DOMAIN_EVENT_H__

#include "utils/type_id_generator.h"
#include <sequence.h>
#include <memory>
#include <typeinfo>
#include <utility>


namespace cddd {
namespace cqrs {

typedef utils::type_id_generator::type_id event_type_id;

class domain_event {
public:
   virtual ~domain_event() noexcept = 0;

   virtual event_type_id type() const = 0;
   virtual std::size_t version() const = 0;
};


inline domain_event::~domain_event() noexcept {}


template<class Evt>
class basic_domain_event final : public domain_event {
public:
   using event_type = std::remove_const_t<std::remove_reference_t<Evt>>;

   explicit constexpr basic_domain_event(Evt &&e, std::size_t ver_) :
      evt(std::forward<Evt>(e)),
      ver(ver_)
   {
   }

   virtual ~basic_domain_event() final {}

   virtual event_type_id type() const final override {
      return utils::type_id_generator::get_id_for_type<event_type>();
   }

   virtual std::size_t version() const final override {
      return ver;
   }

   constexpr const event_type & event() const {
      return evt;
   }

private:
   const event_type evt;
   const std::size_t ver;
};


template<class Evt>
inline bool is_event(const domain_event &evt) {
   return evt.type() == utils::type_id_generator::get_id_for_type<Evt>();
}


template<class Evt>
inline const Evt & unsafe_event_cast(const domain_event &evt) {
   return static_cast<const basic_domain_event<Evt> &>(evt).event();
}


template<class Evt>
inline const Evt & safe_event_cast(const domain_event &evt) {
   // We'll do a fast check first.  If it passes, then things should be OK
   // to perform the cast.  If it fails, then we'll let c++ type system
   // take over.  As such, if dynamic_cast fails, we'll throw std::bad_cast.
   if (is_event<Evt>(evt)) {
      return unsafe_event_cast<Evt>(evt);
   }
   return dynamic_cast<const basic_domain_event<Evt> &>(evt).event();
}

}
}

#endif
