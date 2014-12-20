#ifndef CDDD_MESSAGING_DISPATCHER_H__
#define CDDD_MESSAGING_DISPATCHER_H__

#include "messaging/dispatching_error_policies.h"
#include "utils/table_policies.h"
#include <memory>

namespace cddd {
namespace messaging {

namespace details_ {

class basic_message_handler {
public:
   virtual message_type_id translation_type_id() const = 0;
};


template<class MessageType>
class message_handler : public details_::basic_message_handler {
public:
   virtual bool should_handle(const MessageType &) const = 0;
   virtual bool handle(const MessageType &) = 0;
};


template<class T> struct is_translation_result : std::true_type {};
template<> struct is_translation_result<void> : std::false_type {};
template<> struct is_translation_result<bool> : std::false_type {};
template<> struct is_translation_result<std::error_code> : std::false_type {};
template<> struct is_translation_result<std::error_condition> : std::false_type {};


template<class F> struct is_translator : is_translation_result<message_from_result<F>> {};

}


// TODO: Add support for custom allocation.
template<class ErrorPolicy=return_error_code_on_handling_errors, class DispatchingTablePolicy=utils::use_multimap>
class dispatcher final {
   typedef std::unique_ptr<details_::basic_message_handler> pointer;

public:
   typedef typename ErrorPolicy::result_type result_type;

   template<class MessageHandler, class MessageFilter>
   inline std::enable_if_t<details_::is_translator<MessageHandler>{}(), result_type> add_message_handler(MessageHandler mh, MessageFilter mf) {
      typedef message_from_argument<MessageHandler> message_type;
      static_assert(std::is_convertible<message_type, message_from_argument<MessageFilter>>::value,
                    "Translation function must accept same argument as filter function.");
      static_assert(std::is_same<message_from_result<MessageFilter>, bool>::value,
                    "Filter function must be a predicate (have a return type of bool).");

      class message_handler_impl final : public details_::message_handler<message_type> {
         MessageHandler callback;
         MessageFilter filter;
         dispatcher &self;

      public:
         inline message_handler_impl(MessageHandler &&mh, MessageFilter &&mf, dispatcher &s) :
            details_::message_handler<message_type>(),
            callback(std::move(mh)),
            filter(std::move(mf)),
            self(s)
         {
         }

         virtual bool should_handle(const message_type &m) const final override {
            return filter(m);
         }

         virtual bool handle(const message_type &m) final override {
            return self.do_dispatch_message(translation_type_id(), callback(m)) == make_error_code(dispatching_error::none);
         }

         virtual message_type_id translation_type_id() const final override {
            return utils::type_id_generator::get_id_for_type<message_from_result<MessageHandler>>();
         }
      };

      const message_type_id id = utils::type_id_generator::get_id_for_type<message_type>();
      auto translator = std::make_unique<message_handler_impl>(std::move(mh), std::move(mf), *this);
      auto range = translators.equal_range(id);

      // We want to ensure that only one translator gets registered for a particular translation.
      for (auto i = range.first; i != range.second; ++i) {
         if (i->second->translation_type_id() == translator->translation_type_id()) {
            return ErrorPolicy::failed_to_add_handler(id);
         }
      }

      return save_message_handler(id, std::move(translator), translators);
   }

   template<class MessageHandler, class MessageFilter>
   inline std::enable_if_t<!details_::is_translator<MessageHandler>{}(), result_type> add_message_handler(MessageHandler mh, MessageFilter mf) {
      typedef message_from_argument<MessageHandler> message_type;
      static_assert(std::is_convertible<message_type, message_from_argument<MessageFilter>>::value,
                    "Handling function must accept same argument as filter function.");
      static_assert(std::is_same<message_from_result<MessageFilter>, bool>::value,
                    "Filter function must be a predicate (have a return type of bool).");

      class message_handler_impl final : public details_::message_handler<message_type> {
         MessageHandler callback;
         MessageFilter filter;

      public:
         inline message_handler_impl(MessageHandler &&mh, MessageFilter &&mf) :
            details_::message_handler<message_type>(),
            callback(std::move(mh)),
            filter(std::move(mf))
         {
         }

         virtual bool should_handle(const message_type &m) const final override {
            return filter(m);
         }

         virtual bool handle(const message_type &m) final override {
            return handle_message(m, callback, std::is_same<bool, message_from_result<MessageHandler>>{});
         }

         virtual message_type_id translation_type_id() const final override {
            return 0;
         }
      };

      return save_message_handler(utils::type_id_generator::get_id_for_type<message_type>(),
                                  std::make_unique<message_handler_impl>(std::move(mh), std::move(mf)),
                                  handlers);
   }

   template<class MessageHandler>
   inline result_type add_message_handler(MessageHandler mh) {
      return add_message_handler(std::move(mh), [](const message_from_argument<MessageHandler> &) { return true; });
   }

   template<class MessageType>
   inline result_type dispatch_message(const MessageType &m) {
      const message_type_id id = utils::type_id_generator::get_id_for_type<MessageType>();
      return do_dispatch_message(id, m) == make_error_code(dispatching_error::none) ? ErrorPolicy::success() : ErrorPolicy::no_handlers_found(id, m);
   }

   inline bool has_message_handler(message_type_id id) const {
      return handlers.find(id) != std::end(handlers);
   }

   inline bool has_message_translator(message_type_id id) const {
      return translators.find(id) != std::end(translators);
   }

private:
   typedef typename DispatchingTablePolicy::template table_type<message_type_id, pointer> DispatchingTable;

   template<class MessageType>
   inline std::error_code do_dispatch_message(const message_type_id id, const MessageType &m) {
      const std::error_code success{make_error_code(dispatching_error::none)};
      std::error_code handler_result{dispatch_message_to_handlers(id, m)};
      std::error_code translation_result{dispatch_message_to_translators(id, m)};

      if ((handler_result == success) || (translation_result == success)) {
         return success;
      }

      return make_error_code(dispatching_error::no_handlers_found);
   }

   template<class MessageType, class Callback>
   static inline bool handle_message(const MessageType &m, Callback &callback, std::true_type) {
      return callback(m);
   }

   template<class MessageType, class Callback>
   static inline bool handle_message(const MessageType &m, Callback &callback, std::false_type) {
      (void)callback(m);
      return true;
   }

   static inline result_type save_message_handler(message_type_id id, pointer mh, DispatchingTable &table) {
      bool handler_added = DispatchingTablePolicy::is_insertion_successful(table.emplace(id, std::move(mh)));
      return handler_added ? ErrorPolicy::success() : ErrorPolicy::failed_to_add_handler(id);
   }

   template<class MessageType>
   inline std::error_code dispatch_message_to_handlers(const message_type_id id, const MessageType &m) {
      auto range = handlers.equal_range(id);

      if (range.first == range.second) {
         return make_error_code(dispatching_error::no_handlers_found);
      }

      for (auto i = range.first; i != range.second; ++i) {
         details_::message_handler<MessageType> &handler = static_cast<details_::message_handler<MessageType> &>(*i->second);
         if (handler.should_handle(m)) {
            handler.handle(m);
         }
      }

      return make_error_code(dispatching_error::none);
   }

   template<class MessageType>
   inline std::error_code dispatch_message_to_translators(const message_type_id id, const MessageType &m) {
      auto range = translators.equal_range(id);

      if (range.first == range.second) {
         return make_error_code(dispatching_error::no_handlers_found);
      }

      for (auto i = range.first; i != range.second; ++i) {
         details_::message_handler<MessageType> &translator = static_cast<details_::message_handler<MessageType> &>(*i->second);
         if (translator.should_handle(m) && translator.handle(m)) {
            return make_error_code(dispatching_error::none);
         }
      }

      return make_error_code(dispatching_error::no_handlers_found);
   }

   DispatchingTable handlers;
   DispatchingTable translators;
};

}
}

#endif
