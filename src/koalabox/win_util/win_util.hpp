#pragma once

#include "koalabox/koalabox.hpp"

namespace koalabox::win_util {

    [[maybe_unused]]
    String format_message(DWORD message_id);

    [[maybe_unused]]
    HMODULE get_current_process_handle();

    [[maybe_unused]]
    String get_module_file_name(HMODULE handle);

    [[maybe_unused]]
    MODULEINFO get_module_info(HMODULE module);

    [[maybe_unused]]
    HMODULE get_module_handle(LPCWSTR module_name);

    [[maybe_unused]]
    HMODULE get_module_handle(const String& module_name);

    [[maybe_unused]]
    FARPROC get_proc_address(HMODULE handle, LPCSTR procedure_name);

    [[maybe_unused]]
    Path get_system_directory();

    [[maybe_unused]]
    bool free_library(HMODULE handle);

    [[maybe_unused]]
    HMODULE load_library(const Path& module_path);

    [[maybe_unused]]
    SIZE_T write_process_memory(HANDLE process, LPVOID address, LPCVOID buffer, SIZE_T size);
}
