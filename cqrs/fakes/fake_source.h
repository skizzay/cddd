#ifndef FAKE_SOURCE_H__
#define FAKE_SOURCE_H__

#include "cqrs/source.h"
#include <gmock/gmock.h>

namespace cddd {
namespace cqrs {


template<class T>
class fake_source : public source<T> {
public:
   class spy_type {
   public:
      MOCK_CONST_METHOD1(has, bool(const boost::uuids::uuid &));
      MOCK_CONST_METHOD2_T(get, std::shared_ptr<stream<T>>(const boost::uuids::uuid &, std::size_t));
      MOCK_METHOD1_T(put, void(std::shared_ptr<stream<T>>));
   };

   using source<T>::value_type;

   virtual ~fake_source() = default;

   virtual bool has(const boost::uuids::uuid &id) const final override {
      return spy->has(id);
   }

   virtual std::shared_ptr<stream<T>> get(const boost::uuids::uuid &id, std::size_t version) const final override {
      return spy->get(id, version);
   }

   virtual void put(std::shared_ptr<stream<T>> stream) final override {
      spy->put(stream);
   }

   std::shared_ptr<spy_type> spy=std::make_shared<spy_type>();
};

}
}

#endif
