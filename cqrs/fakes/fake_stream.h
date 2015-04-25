#ifndef FAKE_EVENT_STREAM_H__
#define FAKE_EVENT_STREAM_H__

#include "cqrs/stream.h"
#include "cqrs/commit.h"
#include <deque>

namespace cddd {
namespace cqrs {

template<class T>
class fake_stream : public stream<T> {
public:
   class spy_type {
   public:
      MOCK_CONST_METHOD0(load, void());
      MOCK_CONST_METHOD2(load, void(std::size_t, std::size_t));
      MOCK_METHOD0(save, void());
      MOCK_METHOD0(persist, void());
   };

   using stream<T>::value_type;

   explicit inline fake_stream(std::shared_ptr<spy_type> spy_=std::make_shared<spy_type>()) :
      stream<T>(),
      commitID(),
      sequenceID(),
      version(0),
      sequenceNumber(0),
      time_of_commit(),
      committed_values_script(),
      spy(spy_)
   {
   }
   virtual ~fake_stream() = default;

   virtual sequencing::sequence<T> load(std::size_t min_version, std::size_t max_version) const final override {
      spy->load(min_version, max_version);
      return sequencing::from(committed_values_script)
               | sequencing::where([=](T t) { return min_version <= t->version() && t->version() <= max_version; });
   }

   virtual void save(sequencing::sequence<T> values) final override {
      spy->save();
      std::copy(values.begin(), values.end(), std::back_inserter(committed_values_script));
   }
   
   virtual commit<T> persist() final override {
      spy->persist();
      return commit<T>{commitID, sequenceID, version, sequenceNumber, sequencing::from(committed_values_script), time_of_commit};
   }

   boost::uuids::uuid commitID;
   boost::uuids::uuid sequenceID;
   std::size_t version;
   std::size_t sequenceNumber;
   typename commit<T>::timestamp_type time_of_commit;
   std::deque<T> committed_values_script;
   std::shared_ptr<spy_type> spy;
};


template<class T>
class fake_stream_factory {
public:
   class spy_type {
   public:
      MOCK_CONST_METHOD1_T(create_fake_stream, std::shared_ptr<fake_stream<T>>(const boost::uuids::uuid &));
   };

   inline std::shared_ptr<fake_stream<T>> operator()(const boost::uuids::uuid &id) const {
      return spy->create_fake_stream(id);
   }

   std::shared_ptr<spy_type> spy = std::make_shared<spy_type>();
};

}
}

#endif
