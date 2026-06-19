#include "koalabox/re.hpp"
#include "koalabox/core.hpp"

namespace koalabox::re {
    uintptr_t get_function_start_or_throw(const uintptr_t address) {
        return get_function_start(address) | throw_if_empty(
                   std::format("Failed to find start of function containing address {:#x}", address)
               );
    }
}
