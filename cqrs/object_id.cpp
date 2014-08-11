#include "cqrs/object_id.h"
#include <utility>

namespace cddd {
namespace cqrs {

std::string object_id::to_string() const {
   return is_null() ? std::string() : value->to_string();
}

std::size_t object_id::hash() const {
   return is_null() ? 0 : value->hash();
}

bool object_id::is_null() const {
	return value.read() == nullptr;
}

bool operator==(const object_id &lhs, const object_id &rhs) {
	return !(lhs.is_null() || rhs.is_null()) &&
			lhs.value->equals(*rhs.value);
}

}
}
