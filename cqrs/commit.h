#ifndef CDDD_CQRS_COMMIT_H__
#define CDDD_CQRS_COMMIT_H__

#include "cqrs/exceptions.h"
#include <sequence.h>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace cddd {
namespace cqrs {

template<class T>
class commit {
public:
   typedef boost::posix_time::ptime timestamp_type;
   typedef sequencing::sequence<T> sequence_type;

   explicit inline commit(const boost::uuids::uuid &cid_, const boost::uuids::uuid &sid_, std::size_t version, std::size_t seq,
                          sequence_type values, timestamp_type ts_) :
      cid(cid_),
      sid(sid_),
      revision(version),
      sequence(seq),
      commit_values(std::move(values)),
      ts(ts_)
   {
      if (commit_id().is_nil()) {
         throw null_id_exception{"commit id"};
      }
      else if (stream_id().is_nil()) {
         throw null_id_exception{"stream id"};
      }
      else if (stream_revision() == 0) {
         throw std::out_of_range{"stream revision cannot be 0 (zero)."};
      }
      else if (commit_sequence() == 0) {
         throw std::out_of_range{"commit sequence cannot be 0 (zero)."};
      }
      else if (events().empty()) {
         throw std::invalid_argument{"events cannot be empty."};
      }
      else if (timestamp().is_not_a_date_time()) {
         throw std::invalid_argument{"timestamp is invalid."};
      }
   }

   inline const boost::uuids::uuid &commit_id() const {
      return cid;
   }

   inline const boost::uuids::uuid &stream_id() const {
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

   inline const timestamp_type &timestamp() const {
      return ts;
   }

private:
   boost::uuids::uuid cid;
   boost::uuids::uuid sid;
   std::size_t revision;
   std::size_t sequence;
   sequence_type commit_values;
   timestamp_type ts;
};

}
}

#endif
