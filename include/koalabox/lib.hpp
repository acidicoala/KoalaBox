#pragma once

#include <filesystem>
#include <optional>
#include <set>

#define KB_LIB_GET_FUNC(MODULE, PROC_NAME) \
    koalabox::lib::get_function(MODULE, #PROC_NAME, PROC_NAME)

/// Cross-platform abstraction over dynamic libraries (.dll / .so files)
namespace koalabox::lib {
    // TODO: Refactor into an enum
    constexpr auto CODE_SECTION = ".text";
#if defined(KB_WIN)
    constexpr auto CONST_STR_SECTION = ".rdata";
#elif defined(KB_LINUX)
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

    // TODO: Turns this into a Library class?
    std::filesystem::path get_fs_path(void* module_handle);
    std::optional<void*> get_function_address(void* lib_handle, const char* function_name);
    void* get_function_address_or_throw(void* module_handle, const char* function_name);

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

    // Symbol name as it appears in symbol/export table
    using symbol_name_t = std::string;
    using exports_t = std::set<symbol_name_t>;
#if defined(KB_WIN)
    // On Windows we can get exports from a loaded library
    // TODO: Maybe we should get exports from filesystem as well?
    std::optional<exports_t> get_exports(void* lib_handle);
    exports_t get_exports_or_throw(void* lib_handle);
#elif defined(KB_LINUX)
    // On Linux we can get exports only from a library on filesystem
    std::optional<exports_t> get_exports(const std::filesystem::path& lib_path);
    exports_t get_exports_or_throw(const std::filesystem::path& lib_path);
#endif

    /**
     * Appends "_o" to library name and attempts to load it from the from_path
     */
    void* load_original_library(const std::filesystem::path& from_path, const std::string& lib_name);

    enum class Bitness: uint8_t { $32 = 32U, $64 = 64U };

    std::optional<Bitness> get_bitness(const std::filesystem::path& library_path);
    std::optional<bool> is_32bit(const std::filesystem::path& library_path);
    std::optional<bool> is_64bit(const std::filesystem::path& library_path);
}
