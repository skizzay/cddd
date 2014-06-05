#include "cddd/event_engine/serial_executor.h"

namespace cddd {
namespace event_engine {

serial_executor::serial_executor(executor *e) :
   executor(),
   executor_(e)
{
}


serial_executor::~serial_executor() {
}


void serial_executor::add(std::function<void()> closure) {
   executor_->add(std::move(closure));
}


std::size_t serial_executor::num_pending_closures() const {
   return executor_->num_pending_closures();
}

}
}
