#pragma once

#include <filesystem>
#include <map>

#define KB_MOD_GET_FUNC(MODULE, PROC_NAME) \
    koalabox::lib::get_function(MODULE, #PROC_NAME, PROC_NAME)

/// Cross-platform abstraction over dynamic libraries (.dll / .so files)
namespace koalabox::lib {
    // TODO: Refactor into an enum
    constexpr auto CODE_SECTION = ".text";
#ifdef KB_WIN
    constexpr auto CONST_STR_SECTION = ".rdata";
#elifdef KB_LINUX
    constexpr auto CONST_STR_SECTION = ".rodata";
#endif

    struct section_t {
        void* start_address;
        void* end_address;
        uint32_t size;

        [[nodiscard]] std::string to_string() const {
            return {static_cast<char*>(start_address), size};
        }
    };

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
    void* get_library_handle(const std::string& library_name);

    using undecorated_name = std::string;
    using decorated_name = std::string;
    using export_map_t = std::map<undecorated_name, decorated_name>;

    export_map_t get_export_map(const void* library, [[maybe_unused]] bool undecorate = false);

    std::string get_decorated_function(const void* library, const std::string& function_name);

    /**
     * Appends "_o" to library name and attempts to load it from the from_path
     */
    void* load_original_library(const std::filesystem::path& from_path, const std::string& lib_name);
}
