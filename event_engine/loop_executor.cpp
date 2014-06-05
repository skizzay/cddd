#include "cddd/event_engine/loop_executor.h"

namespace cddd {
namespace event_engine {

loop_executor::loop_executor() :
   executor(),
   shutting_down(false),
   running(ATOMIC_FLAG_INIT),
   lock_on_queue(),
   queue_wakeup()
{
}


loop_executor::~loop_executor() {
   make_loop_exit();
   clear();
   wait_to_complete();
}


void loop_executor::add(std::function<void()> closure) {
   std::unique_lock<std::mutex> lock(lock_on_queue);
   executions.push(closure);
   queue_wakeup.notify_one();
}


std::size_t loop_executor::num_pending_closures() const {
   std::unique_lock<std::mutex> lock(lock_on_queue);
   return executions.size();
}


void loop_executor::loop() {
   if (running.test_and_set()) {
      return;
   }
   shutting_down = false;

   while (!is_shutting_down()) {
      std::unique_lock<std::mutex> lock(lock_on_queue);
      queue_wakeup.wait(lock, std::bind(&loop_executor::is_queue_ready, this));

      if (!is_shutting_down()) {
         std::function<void()> f = executions.front();
         executions.pop();
         lock.unlock();
         try {
            f();
         }
         catch (...) {
            running.clear();
            // Is it correct to rethrow?
            throw;
         }
      }
   }

   running.clear();
}


void loop_executor::run_queued_closures() {
   if (running.test_and_set()) {
      return;
   }

   std::unique_lock<std::mutex> lock(lock_on_queue);
   shutting_down = false;

   for (std::size_t num_executions_to_run = executions.size(); !is_shutting_down() && num_executions_to_run > 0;
        --num_executions_to_run) {
         std::function<void()> f = executions.front();
         executions.pop();
         lock.unlock();
         try {
            f();
         }
         catch (...) {
            running.clear();
            // Is it correct to rethrow?
            throw;
         }
   }

   running.clear();
}


bool loop_executor::try_run_one_closure() {
   if (running.test_and_set()) {
      return false;
   }

   bool result = false;
   std::unique_lock<std::mutex> lock(lock_on_queue);
   shutting_down = false;

   if (is_queue_ready()) {
      std::function<void()> f = executions.front();
      executions.pop();
      lock.unlock();
      try {
         f();
      }
      catch (...) {
         running.clear();
         // Is it correct to rethrow?
         throw;
      }
      result = true;
   }

   running.clear();
   return result;
}


void loop_executor::make_loop_exit() {
   shutting_down = true;
}


void loop_executor::wait_to_complete() {
   // spin until we're no longer running
   while (running.test_and_set()) ;
   running.clear();
}


void loop_executor::clear() {
   std::unique_lock<std::mutex> lock(lock_on_queue);

   while (!executions.empty()) {
      executions.pop();
   }

   queue_wakeup.notify_one();
}

}
}
