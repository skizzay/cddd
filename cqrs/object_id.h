#ifndef CDDD_CQRS_OBJECT_ID_H__
#define CDDD_CQRS_OBJECT_ID_H__

#include "cddd/cqrs/copy_on_write.h"
#include <functional>
#include <sstream>
#include <string>


namespace cddd {
namespace cqrs {

inline std::string to_string(std::string value) {
	return std::move(value);
}


template<typename U>
inline std::string to_string(const U &value) {
	std::ostringstream oss;

	oss << value;

	return oss.str();
}


class object_id {
public:
	template<typename T>
	static object_id create(T &&t) {
      std::unique_ptr<Value> ptr = std::make_unique<Implementation<T>>(std::forward(t));
		object_id result(std::move(ptr));
		return result;
	}
	object_id() = default;
	object_id(const object_id &other) = default;
	object_id(object_id &&) = default;
	~object_id() = default;

	object_id & operator =(const object_id &) = default;
	object_id & operator =(object_id &&) = default;

	std::string to_string() const;
	std::size_t hash() const;
	bool is_null() const;

	friend bool operator==(const object_id &lhs, const object_id &rhs);

private:
	class Value {
	public:
		virtual ~Value() = default;

		virtual std::string to_string() const = 0;
		virtual std::size_t hash() const = 0;
		virtual bool equals(const Value &rhs) const = 0;
		virtual std::unique_ptr<Value> clone() const = 0;
	};

   template<class T>
   class Implementation : public object_id::Value {
   public:
      explicit Implementation(T t) :
         data(std::move(t))
      {
      }

      virtual ~Implementation() = default;

      virtual std::string to_string() const {
         return to_string(this->data);
      }

      virtual std::size_t hash() const {
         std::hash<std::decay_t<T>> hash;
         return hash(data);
      }

      virtual bool equals(const object_id::Value &rhs) const {
         const Implementation *rhsImpl = dynamic_cast<const Implementation *>(&rhs);
         return rhsImpl && data == rhsImpl->data;
      }

      virtual std::unique_ptr<Value> clone() const {
         return std::make_unique<Implementation>(*this);
      }

      inline const T &value() const {
         return this->data;
      }

   private:
      std::decay_t<T> data;
   };

   explicit object_id(std::unique_ptr<Value> v) :
      value(std::move(v))
   {
   }

   copy_on_write<Value> value;
};


inline std::ostream & operator<<(std::ostream &os, const object_id &objectId)
{
   if (!objectId.is_null()) {
      os << objectId.to_string();
   }

   return os;
}


typedef std::function<object_id()> object_id_generator;

}
}


namespace std {

template<>
struct hash<cddd::cqrs::object_id>
{
	inline size_t operator()(const cddd::cqrs::object_id &objectId) const
	{
		return objectId.hash();
	}
};

}

#endif
