#include "koalabox/lib.hpp"
#include "koalabox/core.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/path.hpp"

namespace koalabox::lib {
    namespace fs = std::filesystem;

    void* get_function_address_or_throw(
        void* lib_handle,
        const char* function_name
    ) {
        return get_function_address(lib_handle, function_name) | throw_if_empty(
                   std::format("Failed to get address of function '{}' in module {}", function_name, lib_handle)
               );
    }

    section_t get_section_or_throw(void* lib_handle, const std::string& section_name) {
        return get_section(lib_handle, section_name) | throw_if_empty(
                   std::format("Failed to find section '{}' in module {}", section_name, lib_handle)
               );
    }

    void* load_or_throw(const fs::path& library_path) {
        return load(library_path) | throw_if_empty(
                   std::format("Failed to load library at '{}'", path::to_str(library_path))
               );
    }

    void* load_original_library(const fs::path& from_path, const std::string& lib_name) {
#if defined(KB_WIN)
        constexpr auto extension = "_o.dll";
#elif defined(KB_LINUX)
        constexpr auto extension = "_o.so";
#endif
        const auto full_original_library_name = lib_name + extension;
        const auto original_module_path = from_path / full_original_library_name;

        auto* const original_module = load_or_throw(original_module_path);

        LOG_INFO("Loaded original library: '{}'", full_original_library_name);
        LOG_TRACE("Loaded original library from: '{}'", path::to_str(original_module_path));

        return original_module;
    }

    std::optional<bool> is_32bit(const fs::path& library_path) {
        return get_bitness(library_path) == Bitness::$32;
    }

    std::optional<bool> is_64bit(const fs::path& library_path) {
        return get_bitness(library_path) == Bitness::$64;
    }
}
