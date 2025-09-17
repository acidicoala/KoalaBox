#include <string>

#include "koalabox/loader.hpp"
#include "koalabox/logger.hpp"

namespace koalabox::loader {
    std::string get_decorated_function(const void* /*library*/, const std::string& function_name) {
        // No valid use case for this so far
        return function_name;
    }
}
