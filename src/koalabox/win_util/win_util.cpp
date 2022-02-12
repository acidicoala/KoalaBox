#include "win_util.hpp"
#include "../logger/logger.hpp"
#include "../util/util.hpp"

using namespace koalabox;

[[maybe_unused]]
String win_util::format_message(const DWORD message_id) {
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
HMODULE win_util::get_current_process_handle() {
    auto handle = ::GetModuleHandleW(nullptr);

    if (handle == nullptr) {
        util::panic(__func__, "Failed to get a handle of the current process");
    }

    return handle;
}

[[maybe_unused]]
HMODULE win_util::get_module_handle(const String& module_name) {
    auto name = util::to_wstring(module_name);
    return win_util::get_module_handle(name.c_str());
}

[[maybe_unused]]
HMODULE win_util::get_module_handle(LPCWSTR module_name) {
    auto handle = ::GetModuleHandleW(module_name);

    if (handle == nullptr) {
        util::panic(__func__,
            "Failed to get a handle of the '{}' module",
            util::to_string(module_name)
        );
    }

    return handle;
}

[[maybe_unused]]
String win_util::get_module_file_name(const HMODULE handle) {
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
MODULEINFO win_util::get_module_info(const HMODULE module) {
    MODULEINFO module_info = { nullptr };

    auto result = GetModuleInformation(
        GetCurrentProcess(),
        module,
        &module_info,
        sizeof(module_info)
    );

    if (result == 0) {
        util::panic(__func__,
            "Failed to get module info of the given module handle: {}",
            fmt::ptr(module)
        );
    }

    return module_info;
}

[[maybe_unused]]
FARPROC win_util::get_proc_address(const HMODULE handle, LPCSTR procedure_name) {
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
bool win_util::free_library(const HMODULE handle) {
    auto successful = ::FreeLibrary(handle);

    if (not successful) {
        logger::error("{} - Failed to free a library with the given module handle: {}",
            __func__, fmt::ptr(handle)
        );
    }

    return successful;
}

[[maybe_unused]]
HMODULE win_util::load_library(const Path& module_path) {
    auto handle = ::LoadLibraryW(module_path.wstring().c_str());

    if (handle == nullptr) {
        util::panic(__func__,
            "Failed to load the module at path: '{}'",
            module_path.string()
        );
    }

    return handle;
}

[[maybe_unused]]
SIZE_T win_util::write_process_memory(HANDLE process, LPVOID address, LPCVOID buffer, SIZE_T size) {
    SIZE_T bytes_written = -1;

    auto result = WriteProcessMemory(process, address, buffer, size, &bytes_written);


    if (result == 0) {
        util::panic(__func__,
            "Failed to write process memory at address: '{}'",
            fmt::ptr(address)
        );
    }

    return bytes_written;
}
