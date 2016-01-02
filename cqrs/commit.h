#pragma once

#include "utils/validation.h"
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <type_traits>

namespace cddd {
namespace cqrs {

class commit {
   // Create default constructor for noncommits only.
   explicit inline commit() :
      cid(boost::uuids::nil_uuid()),
      sid(boost::uuids::nil_uuid()),
      revision(0),
      sequence(0),
      ts()
   {
   }

public:
   typedef boost::posix_time::ptime timestamp_type;

   static inline commit noncommit() {
      return commit{};
   }

   explicit inline commit(const boost::uuids::uuid &cid_, const boost::uuids::uuid &sid_,
                          std::size_t version, std::size_t seq,
                          timestamp_type ts_) noexcept(std::is_nothrow_copy_constructible<boost::uuids::uuid>::value &&
				                       std::is_nothrow_copy_assignable<timestamp_type>::value) :
      cid(cid_),
      sid(sid_),
      revision(version),
      sequence(seq),
      ts(ts_)
   {
      utils::validate_id(commit_id());
      utils::validate_id(stream_id());
      if (stream_revision() == 0) {
         throw std::out_of_range{"stream revision cannot be 0 (zero)."};
      }
      else if (commit_sequence() == 0) {
         throw std::out_of_range{"commit sequence cannot be 0 (zero)."};
      }
      else if (timestamp().is_not_a_date_time()) {
         throw std::invalid_argument{"timestamp is invalid."};
      }
   }

   inline const boost::uuids::uuid &commit_id() const noexcept {
      return cid;
   }

   inline const boost::uuids::uuid &stream_id() const noexcept {
      return sid;
   }

   inline std::size_t stream_revision() const noexcept {
      return revision;
   }

   inline std::size_t commit_sequence() const noexcept {
      return sequence;
   }

   inline const timestamp_type &timestamp() const noexcept {
      return ts;
   }

   inline bool is_noncommit() const noexcept {
      return cid.is_nil() ||
             sid.is_nil() ||
             revision == 0 ||
             sequence == 0 ||
             ts.is_not_a_date_time();
   }

private:
   boost::uuids::uuid cid;
   boost::uuids::uuid sid;
   std::size_t revision;
   std::size_t sequence;
   timestamp_type ts;
};

}
}
