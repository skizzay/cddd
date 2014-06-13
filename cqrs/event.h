#ifndef CDDD_CQRS_EVENT_H__
#define CDDD_CQRS_EVENT_H__

#include <deque>
#include <memory>
#include <typeindex>
#include <utility>


namespace cddd {
namespace cqrs {

class event {
public:
   virtual ~event() = 0;

   virtual std::type_index type() const = 0;
};


inline event::~event() {}


typedef std::shared_ptr<event> event_ptr;
template<class Alloc> using basic_event_collection = std::deque<event_ptr, Alloc>;
typedef basic_event_collection<std::allocator<event_ptr>> event_collection;


namespace details_ {

template<class Evt>
class event_wrapper : public event {
public:
   explicit inline event_wrapper(Evt e) :
      evt(std::move(e))
   {
   }

   virtual ~event_wrapper() {}

   virtual std::type_index type() const {
      return typeid(Evt);
   }

   Evt evt;
};

}

}
}

#endif
