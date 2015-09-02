#ifndef CDDD_CQRS_DOMAIN_EVENT_H__
#define CDDD_CQRS_DOMAIN_EVENT_H__

#include "utils/type_id_generator.h"
#include <sequence.h>
#include <memory>
#include <utility>


namespace cddd {
namespace cqrs {

typedef utils::type_id_generator::type_id event_type_id;

class domain_event {
public:
   virtual ~domain_event() = 0;

   virtual event_type_id type() const = 0;
   virtual std::size_t version() const = 0;
};


inline domain_event::~domain_event() {}


template<class Evt>
class basic_domain_event final : public domain_event {
public:
   explicit constexpr basic_domain_event(Evt &&e, std::size_t ver_) :
      evt(std::forward<Evt>(e)),
      ver(ver_)
   {
   }

   virtual ~basic_domain_event() final {}

   virtual event_type_id type() const final override {
      return utils::type_id_generator::get_id_for_type<Evt>();
   }

   virtual std::size_t version() const final override {
      return ver;
   }

   constexpr const Evt & event() const {
      return evt;
   }

private:
   const Evt evt;
   const std::size_t ver;
};

}
}

#endif
