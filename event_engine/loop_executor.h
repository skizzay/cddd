#ifndef CDDD_EVENT_ENGINE_LOOP_EXECUTOR_H__
#define CDDD_EVENT_ENGINE_LOOP_EXECUTOR_H__

#include "event_engine/executor.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>


namespace cddd {
namespace event_engine {

class loop_executor : public executor {
public:
   loop_executor();
   virtual ~loop_executor();

   virtual void add(std::function<void()> closure);
   virtual std::size_t num_pending_closures() const;

   void loop();
   void run_queued_closures();
   bool try_run_one_closure();
   void make_loop_exit();

private:
   std::queue<std::function<void()>> executions;
   bool shutting_down;
   std::atomic_flag running;
   mutable std::mutex lock_on_queue;
   std::condition_variable queue_wakeup;

   inline bool is_queue_ready() const {
      return !executions.empty() || is_shutting_down();
   }
   inline bool is_shutting_down() const {
      return shutting_down;
   }

   void wait_to_complete();
   void clear();
};

}
}

#endif
