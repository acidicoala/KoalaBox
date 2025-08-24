#pragma once

#include <filesystem>

namespace koalabox::win {
    namespace fs = std::filesystem;

    PROCESS_INFORMATION create_process(
        const std::string& app_name,
        const std::string& args,
        const fs::path& working_dir,
        bool show_window
    );

    std::string format_message(DWORD message_id);

    std::string get_last_error();

    /**
     * @return Parent directory of the given module
     */
    fs::path get_module_path(const HMODULE& handle);

    HMODULE get_module_handle_or_throw(LPCSTR module_name);

    HMODULE get_module_handle(LPCSTR module_name);

    MODULEINFO get_module_info_or_throw(const HMODULE& module_handle);

    MODULEINFO get_module_info(const HMODULE& module_handle);

    std::string get_module_manifest(const HMODULE& module_handle);

    std::string get_module_version_or_throw(const HMODULE& module_handle);

    PIMAGE_SECTION_HEADER get_pe_section_or_throw(
        const HMODULE& module_handle,
        const std::string& section_name
    );

    std::string get_pe_section_data_or_throw(
        const HMODULE& module_handle,
        const std::string& section_name
    );

    std::string get_pe_section_data(const HMODULE& module_handle, const std::string& section_name);

    FARPROC get_proc_address_or_throw(const HMODULE& module_handle, LPCSTR procedure_name);

    FARPROC get_proc_address(const HMODULE& module_handle, LPCSTR procedure_name);

    fs::path get_system_directory_or_throw();

    fs::path get_system_directory();

    void free_library_or_throw(const HMODULE& module_handle);

    bool free_library(const HMODULE& module_handle, bool panic_on_fail = false);

    HMODULE load_library_or_throw(const fs::path& module_path);

    HMODULE load_library(const fs::path& module_path);

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
}
