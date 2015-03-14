#ifndef FAKE_OBJECT_ID_GENERATOR_H__
#define FAKE_OBJECT_ID_GENERATOR_H__

#include <boost/uuid/uuid.hpp>
#include <gmock/gmock.h>

class fake_object_id_generator {
public:
   MOCK_METHOD0(generate_id, boost::uuids::uuid());

   boost::uuids::uuid operator()() {
      return generate_id();
   }
};

#endif
