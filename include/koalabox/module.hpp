#pragma once

#include <filesystem>
#include <set>

#define KB_MOD_GET_FUNC(MODULE, PROC_NAME) \
    koalabox::module::get_function(MODULE, #PROC_NAME, PROC_NAME)

/// Cross-platform abstraction over dynamic libraries (.DLL / .SO files)
namespace koalabox::module {
    // TODO: Refactor into an enum
    constexpr auto CONST_STR_SECTION = ".rodata";
#ifdef _WIN32
    constexpr auto CONST_STR_SECTION = ".rdata";
#else
    constexpr auto CODE_SECTION = ".text";
#endif

    struct section_t {
        void* start_address;
        void* end_address;
        uint32_t size;

        [[nodiscard]] std::string to_string() const {
            return {static_cast<char*>(start_address), size};
        }
    };

    using exports_t = std::set<std::string>;
    exports_t get_exports(const std::filesystem::path& lib_path);

    std::filesystem::path get_fs_path(void* module_handle);
    std::optional<void*> get_function_address(
        void* module_handle,
        const char* function_name
    );
    void* get_function_address_or_throw(
        void* module_handle,
        const char* function_name
    );

    template<typename F>
    F get_function(void* module_handle, const char* procedure_name, F) {
        return reinterpret_cast<F>(get_function_address_or_throw(module_handle, procedure_name));
    }

    std::optional<section_t> get_section(void* lib_handle, const std::string& section_name);
    section_t get_section_or_throw(void* module_handle, const std::string& section_name);

    std::optional<void*> load_library(const std::filesystem::path& library_path);
    void* load_library_or_throw(const std::filesystem::path& library_path);
    void unload_library(void* library_handle);
    void* get_library_handle(const TCHAR* library_name);
}
