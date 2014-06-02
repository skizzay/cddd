#ifndef CDDD_EVENT_ENGINE_EXECUTOR_H__
#define CDDD_EVENT_ENGINE_EXECUTOR_H__

#include <chrono>
#include <functional>
#include <memory>

namespace cddd {
namespace event_engine {

class executor {
public:
   virtual ~executor() = default;

   virtual void add(std::function<void()> closure) = 0;
   virtual std::size_t num_pending_closures() const = 0;
};


class scheduled_executor : public executor {
public:
   virtual void add_at(std::chrono::system_clock::time_point abs_time,
                       std::function<void()> closure) = 0;
   virtual void add_after(std::chrono::system_clock::duration rel_time,
                       std::function<void()> closure) = 0;
};


scheduled_executor *default_executor();
void set_default_executor(scheduled_executor *executor_);

}
}


#endif
