#ifndef CDDD_CQRS_EVENT_H__
#define CDDD_CQRS_EVENT_H__

#include <sequence.h>
#include <memory>
#include <typeindex>
#include <utility>


namespace cddd {
namespace cqrs {

class event {
public:
   virtual ~event() = 0;

   virtual std::type_index type() const = 0;
   virtual std::size_t version() const = 0;
};


inline event::~event() {}


typedef std::shared_ptr<event> event_ptr;
typedef std::experimental::sequence<event_ptr> event_sequence;


namespace details_ {

template<class Evt>
class event_wrapper : public event {
public:
   explicit constexpr event_wrapper(Evt e, std::size_t ver_) :
      evt(std::move(e)),
      ver(ver_)
   {
   }

   virtual ~event_wrapper() {}

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
