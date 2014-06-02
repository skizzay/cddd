#ifndef CDDD_MESSAGING_MESSAGE_H__
#define CDDD_MESSAGING_MESSAGE_H__

namespace cddd {
namespace messaging {

class message {
public:
   virtual ~message() = 0;
};


inline message::~message() {
}


class message_handler {
public:
   virtual ~message_handler() = default;

   virtual void handle(message const & msg) = 0;
};

}
}

#endif
