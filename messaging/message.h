#ifndef CDDD_MESSAGING_MESSAGE_H__
#define CDDD_MESSAGING_MESSAGE_H__

//#include "messaging/header.h"
#include <algorithm>
#include <deque>
#include <string>
#include <tuple>

namespace cddd {
namespace messaging {

typedef std::tuple<std::string, std::string> header;

class message {
public:
   typedef std::deque<header> header_collection_type;

   virtual ~message() = 0;

   inline const header_collection_type & headers() const {
      return hdrs;
   }

   inline bool has_header(const header::name_type &name) const {
      return find_header_by_name(name) != std::end(headers());
   }

   // If no header exists with name, then behavior is undefined.
   inline const header &get_header(const header::name_type &name) const {
      return *find_header_by_name(name);
   }

   void add_header(header hdr) {
      hdrs.push_back(std::move(hdr));
   }

protected:
   inline message(header_collection_type h=header_collection_type{}) :
      hdrs(std::move(h))
   {
   }
   inline message(const message &) = default;
   inline message(message &&) = default;

private:
   inline header_collection_type::const_iterator find_header_by_name(const header::name_type &name) const {
      using std::begin;
      using std::end;
      using std::find_if;
      return find_if(begin(headers()), end(headers()), [&name](const header &h) { return name == h.name; });
   }

   header_collection_type hdrs;
};

inline message::~message() {}


template<class T>
class basic_message : public message {
public:
   typedef T payload_type;

   explicit constexpr basic_message(payload_type pt, header_collection_type h=header_collection_type{}) :
      message(),
      content(std::move(pt))
   {
   }

   virtual ~basic_message() = default;

   constexpr const payload_type &payload() const {
      return content;
   }

private:
   payload_type content;
};

}
}

#endif
