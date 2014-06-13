#ifndef CDDD_CQRS_EVENT_STREAM_H__
#define CDDD_CQRS_EVENT_STREAM_H__

#include "cddd/cqrs/commit.h"
#include <boost/asio/spawn.hpp>


namespace cddd {
namespace cqrs {

template<class Alloc>
class basic_event_stream {
public:
   typedef typename basic_event_collection<Alloc>::allocator_type allocator_type;

   virtual ~basic_event_stream() = default;

   virtual const object_id & id() const = 0;
   virtual std::size_t revision() const = 0;
   virtual void add_event(std::shared_ptr<event> evt) = 0;
   virtual std::unique_ptr<commit> commit_events(object_id commit_id) = 0;
   virtual void clear_changes() = 0;

   virtual const basic_event_collection<Alloc> & committed_events() const = 0;
};


template<class Alloc>
class basic_async_event_stream : public basic_event_stream<Alloc>,
                                 public std::enable_shared_from_this<basic_async_event_stream<Alloc>> {
public:
   using typename basic_event_stream<Alloc>::allocator_type;

   explicit inline basic_async_event_stream(boost::asio::io_service &service) :
      basic_event_stream<Alloc>(),
      std::enable_shared_from_this<basic_async_event_stream<Alloc>>(),
      strand(service)
   {
   }

   virtual inline std::unique_ptr<commit> commit_events(object_id commit_id) final override {
      std::unique_ptr<commit> result;
      commit_events(commit_id, [&result](std::unique_ptr<commit> c) { result = std::move(c); });
      return std::move(result);
   }

   template<class OnCommitted>
   inline void commit_events(object_id commit_id, OnCommitted on_committed) {
      auto self = this->shared_from_this();
      boost::asio::spawn(strand, [=](auto yield) {
            on_committed(self->commit_events(commit_id, yield));
         });
   }

   virtual std::unique_ptr<commit> commit_events(object_id commit_id, boost::asio::yield_context yield) = 0;

private:
   boost::asio::io_service::strand strand;
};


typedef basic_event_stream<std::allocator<event_ptr>> event_stream;
typedef basic_async_event_stream<std::allocator<event_ptr>> async_event_stream;

}
}

#endif
