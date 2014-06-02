#ifndef CDDD_CQRS_AGGREATE_H__
#define CDDD_CQRS_AGGREATE_H__

#include "cddd/cqrs/event.h"
#include "cddd/cqrs/exceptions.h"
#include "cddd/event_engine/clock.h"

namespace cddd {
namespace cqrs {

class commit {
public:
   using cddd::event_engine::clock::time_point;

   explicit inline commit(object_id cid_, object_id sid_, std::size_t version, std::size_t seq,
                          event_collection evts, time_point ts_) :
      cid(cid_),
      sid(sid_),
      revision(version),
      sequence(seq),
      commit_events(std::move(evts)),
      ts(ts_)
   {
      if (commit_id().is_null()) {
         throw null_id_exception("commit id");
      }
      else if (stream_id().is_null()) {
         throw null_id_exception("stream id");
      }
      else if (stream_revision() == 0) {
         throw std::out_of_range("stream revision cannot be 0 (zero).");
      }
      else if (commit_sequence() == 0) {
         throw std::out_of_range("commit sequence cannot be 0 (zero).");
      }
      else if (events().empty()) {
         throw std::invalid_argument("events cannot be empty.");
      }
   }

   inline const object_id & commit_id() const {
      return cid;
   }

   inline const object_id & stream_id() const {
      return sid;
   }

   inline std::size_t stream_revision() const {
      return revision;
   }

   inline std::size_t commit_sequence() const {
      return sequence;
   }

   inline const event_collection & events() const {
      return commit_events;
   }

   inline time_point timestamp() const {
      return ts;
   }

private:
   object_id cid;
   object_id sid;
   std::size_t revision;
   std::size_t sequence;
   event_collection commit_events;
   time_point ts;
};

}
}

#endif
