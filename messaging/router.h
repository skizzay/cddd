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

   static inline void success() {}
};


struct ignore_routing_errors {
   typedef void result_type;

   template<class MessageType>
   static inline void no_routes_found(message_type_id, const MessageType &) {
   }

   static inline void no_translator_found(message_type_id) {
   }

   static inline void translator_exists(message_type_id) {
   }

   static inline void success() {}
};


struct return_boolean_value_on_routing_errors {
   typedef bool result_type;

   template<class MessageType>
   static inline result_type no_routes_found(message_type_id, const MessageType &) {
      return false;
   }

   static inline result_type no_translator_found(message_type_id) {
      return false;
   }

   static inline result_type translator_exists(message_type_id) {
      return false;
   }

   static inline result_type success() {
      return true;
   }
};


namespace details_ {

template<class F> using message_from_argument = std::decay_t<typename utils::function_traits<F>::template argument<0>::type>;
template<class F> using message_from_result = std::decay_t<typename utils::function_traits<F>::result_type>;


class message_handler {};


template<class T>
class typed_message_handler : public message_handler,
                              private utils::type_id_generator {
public:
   virtual void handle(const T &t) = 0;

   static inline message_type_id type_id() {
      return get_id_for_type<T>();
   }
};


template<class T, class Callback>
class wrapped_message_handler final : public typed_message_handler<T> {
   Callback callback;

public:
   explicit inline wrapped_message_handler(Callback &&cb) :
      typed_message_handler<T>(),
      callback(std::forward<Callback>(cb))
   {
   }

   virtual void handle(const T &t) final override {
      callback(t);
   }
};


template<class ErrorPolicy>
class data_handler {
public:
   typedef typename ErrorPolicy::result_type result_type;

   virtual bool should_handle(const void *data, std::size_t data_size) const = 0;
   virtual void handle(const void *data, std::size_t data_size) = 0;

   inline void add_message_handler(std::unique_ptr<message_handler> route) {
      routes.emplace_back(std::move(route));
   }

protected:
   template<class Message>
   inline result_type route(Message &&m) {
      if (routes.empty()) {
         return ErrorPolicy::no_routes_found(typed_message_handler<Message>::type_id(), m);
      }

      for (auto &r : routes) {
         static_cast<typed_message_handler<Message> *>(r.get())->handle(m);
      }

      return ErrorPolicy::success();
   }

   std::vector<std::unique_ptr<message_handler>> routes;
};


template<class Translator, class TranslationDeterminationPredicate, class ErrorPolicy>
class typed_data_handler final : public data_handler<ErrorPolicy> {
public:
   inline typed_data_handler(Translator &&t, TranslationDeterminationPredicate &&tdp) :
      data_handler<ErrorPolicy>(),
      translate(std::forward<Translator>(t)),
      should_handle_content(std::forward<TranslationDeterminationPredicate>(tdp))
   {
   }

   virtual bool should_handle(const void *data, std::size_t data_size) const final override {
      return should_handle_content(data, data_size);
   }

   virtual void handle(const void *data, std::size_t data_size) final override {
      this->route(translate(data, data_size));
   }

private:
   Translator translate;
   TranslationDeterminationPredicate should_handle_content;
};

}


template<class ErrorPolicy=throw_exception_on_routing_errors>
class basic_router final {
public:
   typedef typename ErrorPolicy::result_type result_type;

   template<class Fun>
   inline result_type add_route(Fun &&f) {
      typedef details_::message_from_argument<Fun> message_type;
      typedef details_::wrapped_message_handler<message_type, std::decay_t<Fun>> handler_type;

      const message_type_id id = handler_type::type_id();
      if (!this->has_data_handler(id)) {
         return ErrorPolicy::no_translator_found(id);
      }

      auto &r = routes[id];
      r->add_message_handler(std::make_unique<handler_type>(std::forward<Fun>(f)));
      return ErrorPolicy::success();
   }

   template<class Fun, class Filter>
   inline result_type add_route(Fun &&f, Filter &&filter) {
      typedef details_::message_from_argument<Fun> message_type;
      static_assert(std::is_convertible<message_type, details_::message_from_argument<Filter>>::value,
                    "Route function must accept same argument as filter function.");

      return this->add_route([process=std::forward<Fun>(f), allowed_to_process_message=std::forward<Filter>(filter)](const message_type &message) {
            if (allowed_to_process_message(message)) {
               process(message);
            }
         });
   }

   template<class Fun, class Pred>
   inline result_type add_translator(Fun &&f, Pred &&p) {
      typedef details_::message_from_result<Fun> message_type;
      typedef details_::typed_data_handler<Fun, Pred, ErrorPolicy> data_handler_type;

      const message_type_id id = details_::typed_message_handler<message_type>::type_id();
      bool route_added = false;

      std::tie(std::ignore, route_added) = routes.emplace(id, std::make_unique<data_handler_type>(std::forward<Fun>(f), std::forward<Pred>(p)));
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

   std::map<message_type_id, std::unique_ptr<details_::data_handler<ErrorPolicy>>> routes;
};


typedef basic_router<> router;

}
}

#endif
