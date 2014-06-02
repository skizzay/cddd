#ifndef CDDD_EVENT_ENGINE_LOOP_EXECUTOR_H__
#define CDDD_EVENT_ENGINE_LOOP_EXECUTOR_H__

#include "cddd/event_engine/loop_executor.h"

namespace cddd {
namespace event_engine {

class loop_executor : public executor {
public:
   loop_executor();
   virtual ~loop_executor() = default;

   virtual void add(std::function<void()> closure);
   virtual std::size_t num_pending_closures() const;

   void loop();
   void run_queued_closures();
   bool try_run_one_closure();
   void make_loop_exit();
};

}
}

#endif
