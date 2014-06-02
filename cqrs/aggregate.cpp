#include "cddd/cqrs/aggregate.h"

namespace cddd {
namespace cqrs {

basic_aggregate::basic_aggregate(object_id id_, std::size_t version, std::shared_ptr<event_dispatcher> dispatcher_) :
      aggregate(),
      aggregate_id(id_),
      aggregate_version(version),
      pending_events(),
      dispatcher(dispatcher_)
{
}


basic_aggregate::basic_aggregate(object_id id_, std::shared_ptr<event_dispatcher> dispatcher_) :
      basic_aggregate(id_, 0, dispatcher_)
{
}


const object_id & basic_aggregate::id() const {
   return aggregate_id;
}


std::size_t basic_aggregate::revision() const {
   return aggregate_version;
}


const event_collection &basic_aggregate::uncommitted_events() const {
   return pending_events;
}


bool basic_aggregate::has_uncommitted_events() const {
   return !uncommitted_events().empty();
}


void basic_aggregate::clear_uncommitted_events() {
   pending_events.clear();
}


void basic_aggregate::apply_change(std::shared_ptr<event> evt) {
   apply_change(evt, true);
}


void basic_aggregate::apply_change(std::shared_ptr<event> evt, bool is_new) {
   if (evt) {
      dispatcher->dispatch(evt);
      ++aggregate_version;
      if (is_new) {
         pending_events.push_back(evt);
      }
   }
}

}
}
