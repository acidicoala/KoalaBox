#include "win_util.hpp"
#include "logger/logger.hpp"
#include "util/util.hpp"

using namespace koalabox;

[[maybe_unused]]
String win_util::format_message(DWORD message_id) {
    wchar_t buffer[1024];

    ::FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        message_id,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        buffer,
        (sizeof(buffer) / sizeof(wchar_t)),
        nullptr
    );

    return util::to_string(buffer);
}

[[maybe_unused]]
HMODULE win_util::get_module_handle(String& module_name) {
    return win_util::get_module_handle(module_name.c_str());
}

[[maybe_unused]]
HMODULE win_util::get_module_handle(LPCSTR module_name) {
    auto handle = ::GetModuleHandleA(module_name);

    if (handle == nullptr) {
        util::panic(__func__,
            "Failed to get a handle of the '{}' module",
            module_name
        );
    }

    return handle;
}

[[maybe_unused]]
String win_util::get_module_file_name(HMODULE handle) {
    constexpr auto buffer_size = 1024;
    WCHAR buffer[buffer_size];
    auto length = ::GetModuleFileNameW(handle, buffer, buffer_size);

    if (length == 0 or length == buffer_size) {
        util::panic(__func__,
            "Failed to get a file name of the given module handle: {}",
            fmt::ptr(handle)
        );
    }

    return util::to_string(WideString(buffer));
}

[[maybe_unused]]
FARPROC win_util::get_proc_address(HMODULE handle, LPCSTR procedure_name) {
    auto address = ::GetProcAddress(handle, procedure_name);

    if (address == nullptr) {
        util::panic(__func__,
            "Failed to get the address of the '{}' procedure",
            procedure_name
        );
    }

    return address;
}

[[maybe_unused]]
Path win_util::get_system_directory() {
    WCHAR path[MAX_PATH];
    auto result = GetSystemDirectoryW(path, MAX_PATH);

    if (result > MAX_PATH) {
        util::panic(__func__,
            "GetSystemDirectoryW path length ({}) is greater than MAX_PATH ({})",
            result, MAX_PATH
        );
    } else if (result == 0) {
        util::panic(__func__, "Failed to get the path of the system directory");
    }

    return std::filesystem::absolute(path);
}

[[maybe_unused]]
bool win_util::free_library(HMODULE handle) {
    auto successful = ::FreeLibrary(handle);

    if (not successful) {
        logger::error(__func__,
            "Failed to free a library with the given module handle: {}",
            fmt::ptr(handle)
        );
    }

    return successful;
}

[[maybe_unused]]
HMODULE win_util::load_library(Path& module_path) {
    auto handle = ::LoadLibraryW(module_path.wstring().c_str());

    if (handle == nullptr) {
        util::panic(__func__,
            "Failed to load the module at path: '{}'",
            module_path.string()
        );
    }

    return handle;
}
