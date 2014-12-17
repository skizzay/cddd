#ifndef CDDD_MESSAGING_DISPATCHING_ERROR_POLICIES_H__
#define CDDD_MESSAGING_DISPATCHING_ERROR_POLICIES_H__

#include "messaging/traits.h"
#include <sstream>
#include <system_error>

namespace cddd {
namespace messaging {

enum class dispatching_error : int {
   none = 0,
   no_handlers_found,
   failed_to_add_handler,
   unknown
};

}
}


// Required specializations
namespace std {

template<>
struct is_error_code_enum<cddd::messaging::dispatching_error> : true_type {};

template<>
struct is_error_condition_enum<cddd::messaging::dispatching_error> : true_type {};

}


namespace cddd {
namespace messaging {

class dispatching_error_category_t final : public std::error_category {
public:
   virtual ~dispatching_error_category_t() noexcept final override = default;

   virtual const char *name() const noexcept final override {
      return "dispatching error";
   }

   virtual std::error_condition default_error_condition(int ev) const noexcept final override {
      return std::error_condition{std::min(ev, static_cast<int>(dispatching_error::unknown)), *this};
   }

   virtual bool equivalent(const std::error_code &code, int condition) const noexcept final override {
      return *this == code.category() &&
             default_error_condition(code.value()).value() == condition;
   }

   virtual std::string message(int ev) const noexcept final override {
      switch (static_cast<dispatching_error>(ev)) {
         case dispatching_error::none:
            return "None";

         case dispatching_error::no_handlers_found:
            return "No handlers found";

         case dispatching_error::failed_to_add_handler:
            return "Failed to add handler";

         case dispatching_error::unknown:
         default:
            return "Unknown";
      }
   }
} dispatching_error_category;


struct return_error_code_on_handling_errors {
   typedef std::error_code result_type;

   template<class MessageType>
   static inline std::error_code no_handlers_found(message_type_id, const MessageType &) noexcept {
      return std::error_code{static_cast<int>(dispatching_error::no_handlers_found), dispatching_error_category};
   }

   static inline std::error_code failed_to_add_handler(message_type_id) noexcept {
      return std::error_code{static_cast<int>(dispatching_error::failed_to_add_handler), dispatching_error_category};
   }

   static inline std::error_code success() noexcept {
      return std::error_code{static_cast<int>(dispatching_error::none), dispatching_error_category};
   }
};


struct throw_exception_on_handling_errors {
   typedef void result_type;

   template<class MessageType>
   static inline void no_handlers_found(message_type_id id, const MessageType &) {
      std::ostringstream message;
      message << "No handlers found for message type " << id << '.';
      throw std::logic_error(message.str());
   }

   static inline void failed_to_add_handler(message_type_id id) {
      std::ostringstream message;
      message << "A handler for message type " << id << " could not be added to handler table.";
      throw std::logic_error(message.str());
   }

   static inline void success() noexcept {}
};


struct ignore_handling_errors {
   typedef void result_type;

   template<class MessageType>
   static inline void no_handlers_found(message_type_id, const MessageType &) noexcept {}

   static inline void failed_to_add_handler(message_type_id) noexcept {}

   static inline void success() noexcept {}
};


// NOTE: Failures return true.  This is to remain consistent with the error code resulting values.
struct return_boolean_value_on_handling_errors {
   typedef bool result_type;

   template<class MessageType>
   static inline result_type no_handlers_found(message_type_id, const MessageType &) noexcept {
      return true;
   }

   static inline result_type failed_to_add_handler(message_type_id) noexcept {
      return true;
   }

   static inline result_type success() noexcept {
      return false;
   }
};

}
}

#endif
