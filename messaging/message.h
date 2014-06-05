#ifndef CDDD_MESSAGING_MESSAGE_H__
#define CDDD_MESSAGING_MESSAGE_H__

#include <string>
#include <tuple>
#include <vector>

namespace cddd {
namespace messaging {

typedef std::tuple<std::string, std::string> header;
typedef std::vector<header> header_collection;


class message {
public:
   virtual ~message() = default;

   virtual const header_collection &headers() const = 0;
};

}
}

#endif
