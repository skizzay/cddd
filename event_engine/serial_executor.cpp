#include "cddd/event_engine/serial_executor.h"

namespace cddd {
namespace event_engine {

serial_executor::serial_executor() :
   executor(),
   shutting_down(false),
   lock_on_queue(),
   queue_wakeup(),
   execution_thread(std::bind(&serial_executor::run, this))
{
}


serial_executor::~serial_executor() {
   lock_on_queue.lock();
   shutting_down = true;
   while (!executions.empty()) {
      executions.pop();
   }

   queue_wakeup.notify_one();
   lock_on_queue.unlock();
   execution_thread.join();
}


void serial_executor::add(std::function<void()> closure) {
   std::unique_lock<std::mutex> lock(lock_on_queue);
   executions.push(closure);
   queue_wakeup.notify_one();
}


std::size_t serial_executor::num_pending_closures() const {
   std::unique_lock<std::mutex> lock(lock_on_queue);
   return executions.size();
}


void serial_executor::run() {
   while (!is_shutting_down()) {
      std::unique_lock<std::mutex> lock(lock_on_queue);
      queue_wakeup.wait(lock, std::bind(&serial_executor::is_queue_ready, this));

      if (!is_shutting_down()) {
         std::function<void()> execution = executions.front();
         executions.pop();
         lock.unlock();
         execution();
      }
   }
}

}
}
