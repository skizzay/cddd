#ifndef CDDD_CQRS_EVENT_STORE_H__
#define CDDD_CQRS_EVENT_STORE_H__

#include "cddd/cqrs/object_id.h"
#include <boost/asio/spawn.hpp>


namespace cddd {
namespace cqrs {

class event_stream;


class event_store {
public:
   virtual ~event_store() = default;

   virtual std::shared_ptr<event_stream> open_stream(object_id) = 0;
};


class async_event_store : public event_store,
                          public std::enable_shared_from_this<async_event_store> {
public:
   explicit inline async_event_store(boost::asio::io_service &service) :
      event_store(),
      std::enable_shared_from_this<async_event_store>(),
      strand(service)
   {
   }

   virtual inline std::shared_ptr<event_stream> open_stream(object_id id) final override {
      std::shared_ptr<event_stream> result;
      open_stream(id, [&result](std::shared_ptr<event_stream> stream) { result = stream; });
      return result;
   }

   template<class OnStreamOpened>
   inline void open_stream(object_id id, OnStreamOpened on_stream_opened) {
      auto self = shared_from_this();
      boost::asio::spawn(strand, [this, self, id, on_stream_opened](auto yield){
            on_stream_opened(self->open_stream(id, yield));
         });
   }

   virtual std::shared_ptr<event_stream> open_stream(object_id id, boost::asio::yield_context yield) = 0;

private:
   boost::asio::io_service::strand strand;
};

}
}

#endif

