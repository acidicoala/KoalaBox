#include "koalabox/module.hpp"
#include "koalabox/koalabox.hpp"
#include "koalabox/path.hpp"

namespace koalabox::module {
    void* get_function_address_or_throw(
        void* module_handle,
        const char* function_name
    ) {
        return get_function_address(module_handle, function_name) | throw_if_empty(
                   std::format("Failed to get address of function '{}' in module {}", function_name, module_handle)
               );
    }

    section_t get_section_or_throw(void* module_handle, const std::string& section_name) {
        return get_section(module_handle, section_name) | throw_if_empty(
                   std::format("Failed to find section '{}' in module {}", section_name, module_handle)
               );
    }

    void* load_library_or_throw(const std::filesystem::path& library_path) {
        return load_library(library_path) | throw_if_empty(
                   std::format("Failed to load library at '{}'", path::to_str(library_path))
               );
    }
}
