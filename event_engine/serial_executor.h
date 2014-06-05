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
   serial_executor(executor *e);
   virtual ~serial_executor();

   virtual void add(std::function<void()> closure);
   virtual std::size_t num_pending_closures() const;

   inline executor *underlying_executor() {
      return executor_;
   }

private:
   executor *executor_;
};

}
}

#endif
