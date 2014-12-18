#ifndef CDDD_MESSAGING_DISPATCHER_H__
#define CDDD_MESSAGING_DISPATCHER_H__

#include "messaging/dispatching_error_policies.h"
#include "utils/table_policies.h"
#include <memory>

namespace cddd {
namespace messaging {

namespace details_ {

class basic_message_handler {};

}


// TODO: Add support for custom allocation.
template<class ErrorPolicy=return_error_code_on_handling_errors, class DispatchingTablePolicy=utils::use_multimap>
class dispatcher final {
   typedef std::unique_ptr<details_::basic_message_handler> pointer;

   template<class MessageType>
   class message_handler : public details_::basic_message_handler {
   public:
      virtual void handle(const MessageType &) = 0;
   };

public:
   typedef typename ErrorPolicy::result_type result_type;

   template<class MessageHandler>
   inline result_type add_message_handler(MessageHandler mh) {
      typedef message_from_argument<MessageHandler> message_type;

      class message_handler_impl final : public message_handler<message_type> {
         MessageHandler callback;

      public:
         explicit inline message_handler_impl(MessageHandler &&mh) :
            message_handler<message_type>(),
            callback(std::move(mh))
         {
         }

         virtual void handle(const message_type &m) final override {
            callback(m);
         }
      };

      const message_type_id id{utils::type_id_generator::get_id_for_type<message_type>()};
      bool handler_added = DispatchingTablePolicy::is_insertion_successful(handlers.emplace(id, std::make_unique<message_handler_impl>(std::move(mh))));

      return handler_added ? ErrorPolicy::success() : ErrorPolicy::failed_to_add_handler(id);
   }

   template<class MessageHandler, class MessageFilter>
   inline result_type add_message_handler(MessageHandler mh, MessageFilter mf) {
      typedef message_from_argument<MessageHandler> message_type;
      static_assert(std::is_convertible<message_type, message_from_argument<MessageFilter>>::value,
                    "Handling function must accept same argument as filter function.");
      static_assert(std::is_same<message_from_result<MessageFilter>, bool>::value,
                    "Filter function must be a predicate (have a return type of bool).");

      return add_message_handler([handle=std::move(mh), filter=std::move(mf)](const message_type &m) {
            if (filter(m)) {
               handle(m);
            }
         });
   }

   template<class MessageTranslator>
   inline result_type add_message_translator(MessageTranslator mt) {
      typedef message_from_argument<MessageTranslator> message_type;

      return add_message_handler([this, translate=std::move(mt)](const message_type &m) { dispatch_message(translate(m)); });
   }

   template<class MessageTranslator, class TranslationVerifier>
   inline result_type add_message_translator(MessageTranslator mt, TranslationVerifier tv) {
      typedef message_from_argument<MessageTranslator> message_type;

      return add_message_handler([this, translate=std::move(mt)](const message_type &m) { dispatch_message(translate(m)); },
                                 std::move(tv));
   }

   template<class MessageType>
   inline result_type dispatch_message(const MessageType &m) {
      const message_type_id id{utils::type_id_generator::get_id_for_type<MessageType>()};
      auto range = handlers.equal_range(id);

      if (range.first == range.second) {
         return ErrorPolicy::no_handlers_found(id, m);
      }

      for (auto i = range.first; i != range.second; ++i) {
         static_cast<message_handler<MessageType> &>(*i->second).handle(m);
      }

      return ErrorPolicy::success();
   }

   inline bool has_message_handler(message_type_id id) const {
      return handlers.find(id) != std::end(handlers);
   }

private:
   typename DispatchingTablePolicy::template table_type<message_type_id, pointer> handlers;
};

}
}

#endif
