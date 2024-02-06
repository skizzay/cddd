//
// Created by andrew on 1/26/24.
//

#ifndef END_OF_FILE_ERROR_H
#define END_OF_FILE_ERROR_H
#include <stdexcept>

namespace skizzay {

struct end_of_file_error : std::out_of_range {
    using std::out_of_range::out_of_range;
};

} // skizzay

#endif //END_OF_FILE_ERROR_H
