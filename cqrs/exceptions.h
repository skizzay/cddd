#ifndef CDDD_CQRS_EXCEPTIONS_H__
#define CDDD_CQRS_EXCEPTIONS_H__

#include "cddd/cqrs/object_id.h"

#include <stdexcept>
#include <sstream>


namespace cddd {
namespace cqrs {

class null_id_exception : public std::invalid_argument {
public:
   explicit null_id_exception(const std::string &variable_name) :
      std::invalid_argument('\'' + variable_name + "' cannot be a null id.")
   {
   }

   virtual ~null_id_exception() = default;
};


class aggregate_not_found : public std::out_of_range {
public:
   explicit aggregate_not_found(const object_id &id) :
      std::out_of_range("aggregate '" + id.to_string() + "' was not found.")
   {
   }
   virtual ~aggregate_not_found() = default;
};


class timed_out : public std::runtime_error {
public:
   using std::runtime_error::runtime_error;
   virtual ~timed_out() = default;
};

}
}

#endif
