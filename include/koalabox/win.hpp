#pragma once

#include <filesystem>

#define KB_WIN_GET_PROC(MODULE, PROC_NAME) \
    koalabox::win::get_proc(MODULE, #PROC_NAME, PROC_NAME)

EXTERN_C BOOLEAN WINAPI DllMain(HMODULE handle, DWORD reason, LPVOID reserved);

// TODO: Replace *_or_throw functions with a utility function
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

    fs::path get_module_path(const HMODULE& handle);

    HMODULE get_module_handle_or_throw(const std::string& module_name);
    HMODULE get_module_handle(const std::string& module_name);

    HMODULE get_process_handle();

    MODULEINFO get_module_info_or_throw(const HMODULE& module_handle);
    MODULEINFO get_module_info(const HMODULE& module_handle);

    std::string get_module_manifest(const HMODULE& module_handle);

    std::string get_module_version_or_throw(const HMODULE& module_handle);

    PIMAGE_SECTION_HEADER get_pe_section_header_or_throw(
        const HMODULE& module_handle,
        const std::string& section_name
    );

    struct pe_section {
        uintptr_t start_address;
        uintptr_t end_address;
        DWORD size;

        std::string to_string() const {
            return {reinterpret_cast<char*>(start_address), size};
        }
    };

    pe_section get_pe_section_or_throw(
        const HMODULE& module_handle,
        const std::string& section_name
    );

    FARPROC get_proc_address_or_throw(const HMODULE& module_handle, LPCSTR procedure_name);

    FARPROC get_proc_address(const HMODULE& module_handle, LPCSTR procedure_name) noexcept;

    template<typename F>
    F get_proc(const HMODULE& module_handle, const LPCSTR procedure_name, F) {
        return reinterpret_cast<F>(get_proc_address(module_handle, procedure_name));
    }

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
}
