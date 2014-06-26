#ifndef FAKE_OBJECT_ID_GENERATOR_H__
#define FAKE_OBJECT_ID_GENERATOR_H__

#include "cddd/cqrs/object_id.h"
#include <gmock/gmock.h>

class fake_object_id_generator {
public:
   MOCK_METHOD0(generate_id, cddd::cqrs::object_id());

   cddd::cqrs::object_id operator()() {
      return generate_id();
   }
};

#endif
