#ifndef CDDD_MESSAGING_ROUTER_H__
#define CDDD_MESSAGING_ROUTER_H__

#include "cqrs/function_traits.h"
#include <map>
#include <memory>
#include <type_traits>

namespace cddd {
namespace messaging {

typedef std::size_t message_type_id;


struct throw_exception_on_routing_errors {
   template<class MessageType>
   static inline void no_routes_found(message_type_id id, const MessageType &) {
      throw std::runtime_error("No routes found.");
   }
};


struct ignore_routing_errors {
   template<class MessageType>
   static inline void no_routes_found(message_type_id, const MessageType &) {
   }
};


namespace details_ {

class router_base {
   static std::size_t current_id;

protected:
   class handler {};

   template<class T>
   static inline message_type_id get_id_for_type() {
      static const message_type_id id = ++current_id;
      return id;
   }
};

}


template<class ErrorPolicy=throw_exception_on_routing_errors>
class router : public details_::router_base {
   template<class T>
   class typed_handler : public details_::router_base::handler {
   public:
      virtual void handle(const T &t) = 0;
      virtual message_type_id type_id() const = 0;
   };

   template<class T, class Callback>
   class wrapped_handler final : public typed_handler<T> {
      Callback callback;

   public:
      explicit inline wrapped_handler(Callback &&cb) :
         typed_handler<T>(),
         callback(std::forward<Callback>(cb))
      {
      }

      virtual void handle(const T &t) final override {
         callback(t);
      }

      virtual message_type_id type_id() const final override {
         return get_id_for_type<T>();
      }
   };

public:
   template<class T>
   inline void route(const T &msg) {
      static const message_type_id id = get_id_for_type<T>();
      auto routes_found = routes.equal_range(id);
      if (routes_found.first == routes_found.second) {
         ErrorPolicy::no_routes_found(id, msg);
      }
      else {
         for (auto handler = routes_found.first; handler != routes_found.second; ++handler) {
            static_cast<typed_handler<T> &>(*handler->second).handle(msg);
         }
      }
   }

   template<class Fun>
   inline void add_route(Fun &&f) {
      typedef wrapped_handler<std::decay_t<typename cqrs::function_traits<Fun>::template argument<0>::type>, std::decay_t<Fun>> handler_type;

      auto handler = std::make_unique<handler_type>(std::move(f));
      routes.emplace(handler->type_id(), std::move(handler));
   }

   template<class Fun, class Filter>
   inline void add_route(Fun &&f, Filter &&filter) {
      typedef std::decay_t<typename cqrs::function_traits<Fun>::template argument<0>::type> argument_type;

      this->add_route([process=std::forward<Fun>(f), allowed_to_process_message=std::forward<Filter>(filter)](const argument_type &message) {
            if (allowed_to_process_message(message)) {
               process(message);
            }
         });
   }

private:
   std::multimap<message_type_id, std::unique_ptr<details_::router_base::handler>> routes;
};

}
}

#endif
