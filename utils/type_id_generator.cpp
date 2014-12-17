#include "utils/type_id_generator.h"

namespace cddd {
namespace utils {

std::atomic<type_id_generator::type_id> type_id_generator::current_id{0};

}
}
