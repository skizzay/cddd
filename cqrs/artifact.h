#ifndef CDDD_CQRS_ARTIFACT_H__
#define CDDD_CQRS_ARTIFACT_H__

#include "cqrs/domain_event.h"
#include "messaging/dispatcher.h"
#include <deque>
#include <sequence.h>


namespace cddd {
namespace cqrs {

template<class DomainEventDispatcher, class DomainEventContainer>
class basic_artifact {
public:
   typedef std::shared_ptr<domain_event> domain_event_ptr;
   typedef DomainEventDispatcher domain_event_dispatcher_type;
   typedef DomainEventContainer domain_event_container_type;
   typedef typename DomainEventContainer::size_type size_type;

   basic_artifact() = delete;

   inline size_type revision() const {
      return artifact_version;
   }

   inline domain_event_sequence uncommitted_events() const {
      return sequencing::from(std::begin(pending_events), std::end(pending_events));
   }

   inline bool has_uncommitted_events() const {
      return !pending_events.empty();
   }

   inline void clear_uncommitted_events() {
      pending_events.clear();
   }

   inline size_type size_uncommitted_events() const {
      return pending_events.size();
   }

   template<class DomainEventStream>
   inline void load_from_history(const DomainEventStream &stream) {
      for (auto evt : stream.load(next_revision())) {
         apply_change(evt, false);
      }
   }

   inline void apply_change(domain_event_ptr evt) {
      apply_change(evt, true);
   }

   template<class Evt, class Alloc=std::allocator<Evt>>
   inline void apply_change(Evt && e, const Alloc &alloc={}) {
      auto ptr = std::allocate_shared<basic_domain_event<Evt>>(alloc, std::forward<Evt>(e), next_revision());
      apply_change(std::static_pointer_cast<domain_event>(ptr));
   }

protected:
   inline basic_artifact(std::shared_ptr<domain_event_dispatcher_type> dispatcher_ = std::make_shared<domain_event_dispatcher_type>(),
                         domain_event_container_type &&events_ = domain_event_container_type{}) :
      artifact_version(0),
      dispatcher(dispatcher_),
      pending_events(std::forward<domain_event_container_type>(events_)) {
   }

   basic_artifact(basic_artifact &&) = default;
   basic_artifact &operator =(basic_artifact &&) = default;

   basic_artifact(const basic_artifact &) = delete;
   basic_artifact &operator =(const basic_artifact &) = delete;

   template<class Fun>
   inline std::enable_if_t<utils::function_traits<Fun>::arity == 1 &&
                           std::is_base_of<domain_event, messaging::message_from_argument<Fun>>::value> add_handler(Fun f) {
      dispatcher->add_message_handler(std::move(f));
   }

   template<class Fun>
   inline std::enable_if_t<utils::function_traits<Fun>::arity == 1 &&
                           !std::is_base_of<domain_event, messaging::message_from_argument<Fun>>::value> add_handler(Fun f) {
      typedef messaging::message_from_argument<Fun> event_type;
      add_handler([f] (const basic_domain_event<event_type> &evt) {
            f(evt.event());
         });
   }

   template<class Fun>
   inline std::enable_if_t<utils::function_traits<Fun>::arity == 2> add_handle(Fun f) {
      typedef messaging::message_from_argument<Fun> event_type;
      static_assert(std::is_convertible<size_t, typename utils::function_traits<Fun>::template argument_at<1>::type>::value,
                    "Handler's second argument must be size_t to get version.");
      add_handler([f] (const basic_domain_event<event_type> &evt) {
            f(evt.event(), evt.version());
         });
   }

   inline void set_version(size_type version) {
      artifact_version = version;
   }

private:
   inline void apply_change(domain_event_ptr evt, bool is_new) {
      if (evt != nullptr) {
         if (is_new) {
            pending_events.push_back(evt);
         }
         dispatcher->dispatch_message(*evt);
      }
   }

   inline size_type next_revision() const {
      return revision() + size_uncommitted_events() + 1;
   }

   size_type artifact_version;
   std::shared_ptr<domain_event_dispatcher_type> dispatcher;
   domain_event_container_type pending_events;
};


typedef basic_artifact<messaging::dispatcher<>, std::deque<domain_event_ptr>> artifact;

}
}

#endif
