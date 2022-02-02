#pragma once

#include "koalabox.hpp"

#include <Windows.h>

namespace koalabox::win_util {

    [[maybe_unused]]
    String format_message(DWORD message_id);

    [[maybe_unused]]
    String get_module_file_name(HMODULE handle);

    [[maybe_unused]]
    HMODULE get_module_handle(const char* module_name);

    [[maybe_unused]]
    HMODULE get_module_handle(String& module_name);

    [[maybe_unused]]
    FARPROC get_proc_address(HMODULE handle, LPCSTR procedure_name);

    [[maybe_unused]]
    Path get_system_directory();

    [[maybe_unused]]
    bool free_library(HMODULE handle);

    [[maybe_unused]]
    HMODULE load_library(Path& module_path);

}
