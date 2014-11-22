#ifndef CDDD_CQRS_DOMAIN_EVENT_H__
#define CDDD_CQRS_DOMAIN_EVENT_H__

#include <sequence.h>
#include <memory>
#include <typeindex>
#include <utility>


namespace cddd {
namespace cqrs {

class domain_event {
public:
   virtual ~domain_event() = 0;

   virtual std::type_index type() const = 0;
   virtual std::size_t version() const = 0;
};


inline domain_event::~domain_event() {}


typedef std::shared_ptr<domain_event> domain_event_ptr;
typedef std::experimental::sequence<domain_event_ptr> domain_event_sequence;


namespace details_ {

template<class Evt>
class domain_event_wrapper : public domain_event {
public:
   explicit constexpr domain_event_wrapper(Evt e, std::size_t ver_) :
      evt(std::move(e)),
      ver(ver_)
   {
   }

   virtual ~domain_event_wrapper() {}

   virtual std::type_index type() const final override {
      return typeid(Evt);
   }

   virtual std::size_t version() const final override {
      return ver;
   }

   const Evt evt;

private:
   const std::size_t ver;
};

}

}
}

#endif
