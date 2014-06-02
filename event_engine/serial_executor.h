#ifndef CDDD_EVENT_ENGINE_SERIAL_EXECUTOR_H__
#define CDDD_EVENT_ENGINE_SERIAL_EXECUTOR_H__

#include "cddd/event_engine/executor.h"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace cddd {
namespace event_engine {

class serial_executor : public executor {
public:
   serial_executor();
   virtual ~serial_executor();

   virtual void add(std::function<void()> closure);
   virtual std::size_t num_pending_closures() const;

private:
   std::queue<std::function<void()>> executions;
   bool shutting_down;
   mutable std::mutex lock_on_queue;
   std::condition_variable queue_wakeup;
   std::thread execution_thread;

   void run();
   inline bool is_queue_ready() const {
      return !executions.empty() || is_shutting_down();
   }
   bool is_shutting_down() const {
      return shutting_down;
   }
};

}
}

#endif
