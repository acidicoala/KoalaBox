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

    std::filesystem::path get_fs_path(void* lib_handle);
    std::optional<void*> get_function_address(void* lib_handle, const char* function_name);
    void* get_function_address_or_throw(void* lib_handle, const char* function_name);

    template<typename F>
    F get_function(void* lib_handle, const char* procedure_name, F) {
        return reinterpret_cast<F>(get_function_address_or_throw(lib_handle, procedure_name));
    }

    std::optional<section_t> get_section(void* lib_handle, const std::string& section_name);
    section_t get_section_or_throw(void* lib_handle, const std::string& section_name);

    std::optional<void*> load(const std::filesystem::path& library_path);
    void* load_or_throw(const std::filesystem::path& library_path);
    void unload(void* lib_handle);
    void* get_lib_handle(const std::string& lib_name);

    /**
     * Appends "_o" to library name and attempts to load it from the from_path
     */
    void* load_original_library(const std::filesystem::path& from_path, const std::string& lib_name);

    enum class Bitness: uint8_t { $32 = 32U, $64 = 64U };

    std::optional<Bitness> get_bitness(const std::filesystem::path& library_path);
    std::optional<bool> is_32bit(const std::filesystem::path& library_path);
    std::optional<bool> is_64bit(const std::filesystem::path& library_path);

    void* get_exe_handle();
}
