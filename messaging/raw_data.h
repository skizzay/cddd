#ifndef CDDD_MESSAGING_RAW_DATA_H__
#define CDDD_MESSAGING_RAW_DATA_H__

#include <cstdint>

namespace cddd {
namespace messaging {

struct raw_data {
   void *data;
   std::size_t size;
};

}
}

#endif
