#ifndef CDDD_CQRS_EXCEPTIONS_H__
#define CDDD_CQRS_EXCEPTIONS_H__

#include <boost/uuid/uuid.hpp>
#include <sstream>
#include <stdexcept>


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


class null_pointer_exception : public std::invalid_argument {
public:
   explicit null_pointer_exception(const std::string &variable_name) :
      std::invalid_argument('\'' + variable_name + "' cannot be a null pointer.")
   {
   }

   virtual ~null_pointer_exception() = default;
};


class aggregate_not_found : public std::out_of_range {
public:
   explicit aggregate_not_found(const boost::uuids::uuid &id) :
      std::out_of_range("aggregate_not_found")
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

}
}

#endif
