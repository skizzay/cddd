#ifndef CDDD_CQRS_EXCEPTIONS_H__
#define CDDD_CQRS_EXCEPTIONS_H__

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <experimental/string_view>
#include <sstream>
#include <stdexcept>


namespace cddd {
namespace utils {

namespace details_ {

inline std::string malformed_id(const boost::uuids::uuid &id, std::experimental::string_view parameter) {
   using std::move;

   std::ostringstream error;
   error << '\'' << parameter << "' is a malformed id: '" << id << '\'';
   return move(error.str());
}


inline std::string null_value(std::experimental::string_view parameter) {
   using std::move;

   std::ostringstream error;
   error << '\'' << parameter << "' cannot be null.";
   return move(error.str());
}


inline std::string not_found(const boost::uuids::uuid &id, std::experimental::string_view parameter) {
   using std::move;

   std::ostringstream error;
   error << '\'' << parameter << "' = '" << id << "' not found.";
   return move(error.str());
}

}

class malformed_id_exception : public std::invalid_argument {
public:
   explicit malformed_id_exception(const boost::uuids::uuid &id, std::experimental::string_view variable_name) :
      std::invalid_argument{details_::malformed_id(id, variable_name)}
   {
   }

   virtual ~malformed_id_exception() = default;
};


class null_id_exception : public std::invalid_argument {
public:
   explicit null_id_exception(std::experimental::string_view variable_name) :
      std::invalid_argument{details_::null_value(variable_name)}
   {
   }

   virtual ~null_id_exception() = default;
};


class null_pointer_exception : public std::invalid_argument {
public:
   explicit null_pointer_exception(std::experimental::string_view variable_name) :
      std::invalid_argument{details_::null_value(variable_name)}
   {
   }

   virtual ~null_pointer_exception() = default;
};


class aggregate_not_found : public std::out_of_range {
public:
   explicit aggregate_not_found(const boost::uuids::uuid &id, std::experimental::string_view variable_name) :
      std::out_of_range{details_::not_found(id, variable_name)}
   {
      (void)id;
   }
   virtual ~aggregate_not_found() = default;
};


class timed_out : public std::runtime_error {
public:
   using std::runtime_error::runtime_error;
   virtual ~timed_out() = default;
};


class concurrency_exception : public std::runtime_error {
public:
   using std::runtime_error::runtime_error;
   virtual ~concurrency_exception() = default;
};


class event_not_handled : public std::logic_error {
public:
   using std::logic_error::logic_error;
   virtual ~event_not_handled() = default;
};


class entity_exists : public std::invalid_argument {
public:
   using std::invalid_argument;
   virtual ~entity_exists() = default;
};

}
}

#endif
