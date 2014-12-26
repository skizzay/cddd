#ifndef CDDD_CQRS_COMMIT_H__
#define CDDD_CQRS_COMMIT_H__

#include "cqrs/exceptions.h"
#include "event_engine/clock.h"
#include <sequence.h>

namespace cddd {
namespace cqrs {

template<class T>
class commit {
public:
   typedef cddd::event_engine::clock::time_point time_point;
   typedef sequencing::sequence<T> sequence_type;

   explicit inline commit(object_id cid_, object_id sid_, std::size_t version, std::size_t seq,
                          sequence_type values, time_point ts_) :
      cid(cid_),
      sid(sid_),
      revision(version),
      sequence(seq),
      commit_values(std::move(values)),
      ts(ts_) {
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

   inline const object_id &commit_id() const {
      return cid;
   }

   inline const object_id &stream_id() const {
      return sid;
   }

   inline std::size_t stream_revision() const {
      return revision;
   }

   inline std::size_t commit_sequence() const {
      return sequence;
   }

   inline const domain_event_sequence &events() const {
      return commit_values;
   }

   inline time_point timestamp() const {
      return ts;
   }

private:
   object_id cid;
   object_id sid;
   std::size_t revision;
   std::size_t sequence;
   sequence_type commit_values;
   time_point ts;
};

}
}

#endif
