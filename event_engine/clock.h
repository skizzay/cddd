#ifndef CDDD_CLOCK_H__
#define CDDD_CLOCK_H__

#include <chrono>


namespace cddd {
namespace event_engine {

class clock {
public:
   typedef std::chrono::system_clock::time_point time_point;

   virtual ~clock() = default;

   virtual time_point now() const = 0;
};


class system_clock : public clock {
public:
   virtual time_point now() const {
      return std::chrono::system_clock::now();
   }
};

}
}

#endif
