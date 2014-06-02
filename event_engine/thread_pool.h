#ifndef CDDD_EVENT_ENGINE_THREAD_POOL_H__
#define CDDD_EVENT_ENGINE_THREAD_POOL_H__

#include "cddd/event_engine/executor.h"

namespace cddd {
namespace event_engine {

class thread_pool : public scheduled_executor {
public:
   explicit thread_pool(int num_threads);
   virtual ~thread_pool();

   virtual void add(std::function<void()> closure);
   virtual std::size_t num_pending_closures() const;
   virtual void add_at(std::chrono::system_clock::time_point abs_time,
                       std::function<void()> closure);
   virtual void add_after(std::chrono::system_clock::duration rel_time,
                       std::function<void()> closure);
};

}
}

#endif
