#pragma once

#include <koalabox/core.hpp>

#define GET_LAST_ERROR koalabox::win_util::get_last_error

namespace koalabox::win_util {

    KOALABOX_API(PROCESS_INFORMATION) create_process(
        const String& app_name,
        const String& args,
        const Path& working_dir,
        bool show_window
    );

    KOALABOX_API(String) format_message(DWORD message_id);

    KOALABOX_API(String) get_last_error();

    KOALABOX_API(String) get_module_file_name_or_throw(const HMODULE& module_handle);

    KOALABOX_API(String) get_module_file_name(const HMODULE& module_handle);

    KOALABOX_API(HMODULE) get_module_handle_or_throw(LPCSTR module_name);

    KOALABOX_API(HMODULE) get_module_handle(LPCSTR module_name);

    KOALABOX_API(MODULEINFO) get_module_info_or_throw(const HMODULE& module_handle);

    KOALABOX_API(MODULEINFO) get_module_info(const HMODULE& module_handle);

    KOALABOX_API(String) get_module_manifest(const HMODULE& module_handle);

    KOALABOX_API(String) get_module_version_or_throw(const HMODULE& module_handle);

    KOALABOX_API(PIMAGE_SECTION_HEADER) get_pe_section_or_throw(
        const HMODULE& module_handle,
        const String& section_name
    );

    KOALABOX_API(String) get_pe_section_data_or_throw(
        const HMODULE& module_handle,
        const String& section_name
    );

    KOALABOX_API(String) get_pe_section_data(
        const HMODULE& module_handle,
        const String& section_name
    );

    KOALABOX_API(FARPROC) get_proc_address_or_throw(
        const HMODULE& module_handle,
        LPCSTR procedure_name
    );

    KOALABOX_API(FARPROC) get_proc_address(const HMODULE& module_handle, LPCSTR procedure_name);

    KOALABOX_API(Path) get_system_directory_or_throw();

    KOALABOX_API(Path) get_system_directory();

    KOALABOX_API(void) free_library_or_throw(const HMODULE& module_handle);

    KOALABOX_API(bool) free_library(const HMODULE& module_handle, bool panic_on_fail = false);

    KOALABOX_API(HMODULE) load_library_or_throw(const Path& module_path);

    KOALABOX_API(HMODULE) load_library(const Path& module_path);

    KOALABOX_API(void) register_application_restart();

    KOALABOX_API(std::optional<MEMORY_BASIC_INFORMATION>) virtual_query(const void* pointer);

    KOALABOX_API(SIZE_T) write_process_memory_or_throw(
        const HANDLE& process, LPVOID address, LPCVOID buffer, SIZE_T size
    );

    SIZE_T write_process_memory(const HANDLE& process, LPVOID address, LPCVOID buffer, SIZE_T size);

}
