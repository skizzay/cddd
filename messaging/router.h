#ifndef CDDD_MESSAGING_ROUTER_H__
#define CDDD_MESSAGING_ROUTER_H__

#include "utils/function_traits.h"
#include "utils/type_id_generator.h"
#include <map>
#include <memory>
#include <sstream>
#include <vector>

namespace cddd {
namespace messaging {

typedef utils::type_id_generator::type_id message_type_id;


struct throw_exception_on_routing_errors {
   typedef void result_type;

   template<class MessageType>
   static inline void no_routes_found(message_type_id id, const MessageType &) {
      std::ostringstream message;
      message << "No routes found for message type " << id << '.';
      throw std::logic_error(message.str());
   }

   static inline void no_translator_found(message_type_id id) {
      std::ostringstream message;
      if (id != 0) {
         message << "No translator found for message type " << id << '.';
      }
      else {
         message << "No translator found for incoming data.";
      }
      throw std::logic_error(message.str());
   }

   static inline void translator_exists(message_type_id id) {
      std::ostringstream message;
      message << "A translator already exists for message type " << id << '.';
      throw std::logic_error(message.str());
   }

   static inline void success() noexcept {}
};


struct ignore_routing_errors {
   typedef void result_type;

   template<class MessageType>
   static inline void no_routes_found(message_type_id, const MessageType &) noexcept {
   }

   static inline void no_translator_found(message_type_id) noexcept {
   }

   static inline void translator_exists(message_type_id) noexcept {
   }

   static inline void success() noexcept {}
};


struct return_boolean_value_on_routing_errors {
   typedef bool result_type;

   template<class MessageType>
   static inline result_type no_routes_found(message_type_id, const MessageType &) noexcept {
      return false;
   }

   static inline result_type no_translator_found(message_type_id) noexcept {
      return false;
   }

   static inline result_type translator_exists(message_type_id) noexcept {
      return false;
   }

   static inline result_type success() noexcept {
      return true;
   }
};


template<class F> using message_from_argument = std::decay_t<typename utils::function_traits<F>::template argument<0>::type>;
template<class F> using message_from_result = std::decay_t<typename utils::function_traits<F>::result_type>;


template<class ErrorPolicy>
class basic_data_handler {
public:
   typedef typename ErrorPolicy::result_type result_type;

   virtual bool should_handle(const void *data, std::size_t data_size) const = 0;
   virtual result_type handle(const void *data, std::size_t data_size) = 0;
};


template<class ErrorPolicy=throw_exception_on_routing_errors, class RoutingTable=std::map<message_type_id, std::unique_ptr<basic_data_handler<ErrorPolicy>>>>
class basic_router final {
   template<class MessageType>
   class message_handler {
   public:
      virtual void handle(const MessageType &t) = 0;
   };


   template<class MessageType>
   class data_handler : public basic_data_handler<ErrorPolicy> {
   public:
      virtual void add_message_handler(std::unique_ptr<message_handler<MessageType>> handler) = 0;
   };

public:
   typedef typename ErrorPolicy::result_type result_type;

   template<class Fun>
   inline result_type add_route(Fun f) {
      typedef message_from_argument<Fun> message_type;

      class message_handler_impl final : public message_handler<message_type> {
         Fun callback;

      public:
         explicit inline message_handler_impl(Fun &&cb) :
            message_handler<message_type>(),
            callback(std::move(cb))
         {
         }

         virtual void handle(const message_type &m) final override {
            callback(m);
         }
      };

      const message_type_id id = utils::type_id_generator::get_id_for_type<message_type>();
      if (!this->has_data_handler(id)) {
         return ErrorPolicy::no_translator_found(id);
      }

      auto *r = static_cast<data_handler<message_type> *>(routes[id].get());
      r->add_message_handler(std::make_unique<message_handler_impl>(std::move(f)));
      return ErrorPolicy::success();
   }


   template<class Fun, class Filter>
   inline result_type add_route(Fun f, Filter filter) {
      typedef message_from_argument<Fun> message_type;
      static_assert(std::is_convertible<message_type, message_from_argument<Filter>>::value,
                    "Route function must accept same argument as filter function.");
      static_assert(std::is_same<message_from_result<Filter>, bool>::value,
                    "Filter function must be a predicate (have a return type of bool).");

      return this->add_route([process=std::move(f), allowed_to_process_message=std::move(filter)](const message_type &message) {
            if (allowed_to_process_message(message)) {
               process(message);
            }
         });
   }


   template<class Fun, class Pred>
   inline result_type add_translator(Fun f, Pred p) {
      typedef message_from_result<Fun> message_type;

      class data_handler_impl final : public data_handler<message_type> {
      public:
         inline data_handler_impl(Fun &&t, Pred &&tdp) :
            data_handler<message_type>(),
            translate(std::move(t)),
            should_handle_content(std::move(tdp))
         {
         }

         virtual bool should_handle(const void *data, std::size_t data_size) const final override {
            return should_handle_content(data, data_size);
         }

         virtual result_type handle(const void *data, std::size_t data_size) final override {
            return this->route(translate(data, data_size));
         }

         virtual void add_message_handler(std::unique_ptr<message_handler<message_type>> handler) final override {
            routes.emplace_back(std::move(handler));
         }

      private:
         inline result_type route(const message_type &m) {
            if (routes.empty()) {
               return ErrorPolicy::no_routes_found(utils::type_id_generator::get_id_for_type<message_type>(), m);
            }

            for (auto &r : routes) {
               static_cast<message_handler<message_type> *>(r.get())->handle(m);
            }

            return ErrorPolicy::success();
         }

         std::vector<std::unique_ptr<message_handler<message_type>>> routes;
         Fun translate;
         Pred should_handle_content;
      };

      const message_type_id id = utils::type_id_generator::get_id_for_type<message_type>();
      bool route_added = false;

      std::tie(std::ignore, route_added) = routes.emplace(id, std::make_unique<data_handler_impl>(std::forward<Fun>(f), std::forward<Pred>(p)));
      return route_added ? ErrorPolicy::success() : ErrorPolicy::translator_exists(id);
   }


   inline result_type on_data(const void *data, std::size_t data_size) {
      for (auto &route : routes) {
         if (route.second->should_handle(data, data_size)) {
            route.second->handle(data, data_size);
            return ErrorPolicy::success();
         }
      }

      return ErrorPolicy::no_translator_found(0);
   }

private:
   inline bool has_data_handler(message_type_id id) const {
      return routes.find(id) != std::end(routes);
   }

   RoutingTable routes;
};


typedef basic_router<> router;

}
}

#endif
