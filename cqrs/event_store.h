#ifndef CDDD_CQRS_EVENT_STORE_H__
#define CDDD_CQRS_EVENT_STORE_H__

#include "cddd/cqrs/event_stream.h"
#include <boost/asio/spawn.hpp>


namespace cddd {
namespace cqrs {

template<class Alloc>
class basic_event_store {
public:
   typedef typename basic_event_stream<Alloc>::allocator_type allocator_type;
   typedef std::shared_ptr<basic_event_stream<Alloc>> event_stream_ptr;

   virtual ~basic_event_store() = default;

   virtual event_stream_ptr open_stream(object_id) = 0;
};


template<class Alloc>
class basic_async_event_store : public basic_event_store<Alloc>,
                                public std::enable_shared_from_this<basic_async_event_store<Alloc>> {
public:
   using typename basic_event_store<Alloc>::allocator_type;
   using typename basic_event_store<Alloc>::event_stream_ptr;

   explicit inline basic_async_event_store(boost::asio::io_service &service) :
      basic_event_store<Alloc>(),
      std::enable_shared_from_this<basic_async_event_store<Alloc>>(),
      strand(service)
   {
   }

   virtual inline event_stream_ptr open_stream(object_id id) final override {
      event_stream_ptr result;
      open_stream(id, [&result](event_stream_ptr stream) { result = stream; });
      return result;
   }

   template<class OnStreamOpened>
   inline void open_stream(object_id id, OnStreamOpened on_stream_opened) {
      auto self = this->shared_from_this();
      boost::asio::spawn(strand, [this, self, id, on_stream_opened](auto yield){
            on_stream_opened(self->open_stream(id, yield));
         });
   }

   virtual event_stream_ptr open_stream(object_id id, boost::asio::yield_context yield) = 0;

private:
   boost::asio::io_service::strand strand;
};


typedef basic_event_store<event_stream::allocator_type> event_store;
typedef basic_async_event_store<async_event_stream::allocator_type> async_event_store;

}
}

#endif

