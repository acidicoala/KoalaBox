#pragma once

#include <filesystem>

#define DLL_MAIN(...) \
    EXTERN_C [[maybe_unused]] BOOLEAN WINAPI DllMain(__VA_ARGS__)

DLL_MAIN(void* handle, uint32_t reason, void* reserved);

// TODO: Replace *_or_throw functions with a utility function
namespace koalabox::win {
    namespace fs = std::filesystem;

    std::vector<uint8_t> get_module_version_info_or_throw(const HMODULE& module_handle);
    std::optional<std::string> get_version_info_string(
        HMODULE module_handle,
        const std::string& key,
        const std::string& codepage = "040904E4" // 0x409 (English - United States) + 1252 (Windows Latin-1)
    ) noexcept;

    PROCESS_INFORMATION create_process(
        const std::string& app_name,
        const std::string& args,
        const fs::path& working_dir,
        bool show_window
    );

    std::string format_message(DWORD message_id);

    std::string get_last_error();

    fs::path get_module_path(const HMODULE& handle);

    HMODULE get_module_handle_or_throw(const std::string& module_name);
    HMODULE get_module_handle(const std::string& module_name);

    HMODULE get_process_handle();

    MODULEINFO get_module_info_or_throw(const HMODULE& module_handle);
    MODULEINFO get_module_info(const HMODULE& module_handle);

    std::string get_module_manifest(const HMODULE& module_handle);

    std::string get_module_version_or_throw(const HMODULE& module_handle);

    fs::path get_system_directory_or_throw();

    fs::path get_system_directory();

    void register_application_restart();

    std::optional<MEMORY_BASIC_INFORMATION> virtual_query(const void* pointer);

    SIZE_T write_process_memory_or_throw(
        const HANDLE& process,
        LPVOID address,
        LPCVOID buffer,
        SIZE_T size
    );

    SIZE_T write_process_memory(const HANDLE& process, LPVOID address, LPCVOID buffer, SIZE_T size);

    std::string get_env_var(const std::string& key);

    // Such checks are possible only on Windows, since Linux libraries don't include such metadata
    void check_self_duplicates() noexcept;
}
