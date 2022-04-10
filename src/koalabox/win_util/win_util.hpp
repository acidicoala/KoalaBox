#pragma once

#include "koalabox/koalabox.hpp"

namespace koalabox::win_util {

    String format_message(DWORD message_id);

    String get_module_file_name_or_throw(const HMODULE& module);

    String get_module_file_name(const HMODULE& handle);

    HMODULE get_module_handle_or_throw(LPCSTR module_name);

    HMODULE get_module_handle(LPCSTR module_name);

    MODULEINFO get_module_info_or_throw(const HMODULE& module);

    MODULEINFO get_module_info(const HMODULE& module);

    String get_module_version_or_throw(const HMODULE& module);

    String get_pe_section_data_or_throw(const HMODULE& module, const String& section_name);

    String get_pe_section_data(const HMODULE& module, const String& section_name);

    FARPROC get_proc_address_or_throw(const HMODULE& handle, LPCSTR procedure_name);

    FARPROC get_proc_address(const HMODULE& handle, LPCSTR procedure_name);

    Path get_system_directory_or_throw();

    Path get_system_directory();

    void free_library_or_throw(const HMODULE& handle);

    bool free_library(const HMODULE& handle, bool panic_on_fail = false);

    HMODULE load_library_or_throw(const Path& module_path);

    HMODULE load_library(const Path& module_path);

    SIZE_T write_process_memory_or_throw(const HANDLE& process, LPVOID address, LPCVOID buffer, SIZE_T size);

    SIZE_T write_process_memory(const HANDLE& process, LPVOID address, LPCVOID buffer, SIZE_T size);

}
