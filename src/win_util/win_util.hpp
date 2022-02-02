#pragma once

#include "koalabox.hpp"

#include <Windows.h>
#include <Psapi.h>

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
}
