#include "win_util.hpp"

#include "koalabox/util/util.hpp"

namespace koalabox::win_util {

#define PANIC_ON_CATCH(FUNC, ...) \
    try { \
        return FUNC##_or_throw(__VA_ARGS__); \
    } catch (const std::exception& ex) { \
        util::panic(ex.what()); \
    }

    String format_message(const DWORD message_id) {
        TCHAR buffer[1024];

        ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            message_id,
            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
            buffer,
            sizeof(buffer) / sizeof(TCHAR),
            nullptr
        );

        return util::to_string(buffer);
    }

    String get_module_file_name_or_throw(const HMODULE& module) {
        constexpr auto buffer_size = 1024;
        TCHAR buffer[buffer_size];
        const auto length = ::GetModuleFileName(module, buffer, buffer_size);

        return (length > 0 and length < buffer_size)
            ? util::to_string(buffer)
            : throw util::exception(
                "Failed to get a file name of the given module handle: {}. Length: {}",
                fmt::ptr(module), length
            );
    }

    String get_module_file_name(const HMODULE& module) {
        PANIC_ON_CATCH(get_module_file_name, module)
    }

    HMODULE get_module_handle_or_throw(LPCSTR module_name) {
        const auto handle = module_name
            ? ::GetModuleHandle(util::to_wstring(module_name).c_str())
            : ::GetModuleHandle(nullptr);

        return handle
            ? handle
            : throw util::exception("Failed to get a handle of the module: '{}'", module_name);
    }

    HMODULE get_module_handle(LPCSTR module_name) {
        PANIC_ON_CATCH(get_module_handle, module_name)
    }

    MODULEINFO get_module_info_or_throw(const HMODULE& module) {
        MODULEINFO module_info = { nullptr };
        const auto success = ::GetModuleInformation(GetCurrentProcess(), module, &module_info, sizeof(module_info));

        return success
            ? module_info
            : throw util::exception("Failed to get module info of the given module handle: {}", fmt::ptr(module));
    }

    MODULEINFO get_module_info(const HMODULE& module) {
        PANIC_ON_CATCH(get_module_info, module)
    }

    FARPROC get_proc_address_or_throw(const HMODULE& handle, LPCSTR procedure_name) {
        const auto address = ::GetProcAddress(handle, procedure_name);

        return address
            ? address
            : throw util::exception("Failed to get the address of the procedure: '{}'", procedure_name);
    }

    FARPROC get_proc_address(const HMODULE& handle, LPCSTR procedure_name) {
        PANIC_ON_CATCH(get_proc_address, handle, procedure_name)
    }

    Path get_system_directory_or_throw() {
        WCHAR path[MAX_PATH];
        const auto result = GetSystemDirectory(path, MAX_PATH);

        if (result > MAX_PATH) {
            throw util::exception(
                "GetSystemDirectory path length ({}) is greater than MAX_PATH ({})",
                result, MAX_PATH
            );
        } else if (result == 0) {
            throw std::exception("Failed to get the path of the system directory");
        }

        return std::filesystem::absolute(path);
    }

    Path get_system_directory() {
        PANIC_ON_CATCH(get_system_directory)
    }

    void free_library_or_throw(const HMODULE& handle) {
        if (not::FreeLibrary(handle)) {
            throw util::exception("Failed to free a library with the given module handle: {}", fmt::ptr(handle));
        }
    }

    bool free_library(const HMODULE& handle, bool panic_on_fail) {
        try {
            free_library_or_throw(handle);
            return true;
        } catch (std::exception& ex) {
            if (panic_on_fail) {
                util::panic(ex.what());
            }

            return false;
        }
    }

    HMODULE load_library_or_throw(const Path& module_path) {
        const auto module = ::LoadLibrary(module_path.wstring().c_str());

        return module
            ? module
            : throw util::exception("Failed to load the module at path: '{}'", module_path.string());
    }

    HMODULE load_library(const Path& module_path) {
        PANIC_ON_CATCH(load_library, module_path)
    }

    SIZE_T write_process_memory_or_throw(const HANDLE& process, LPVOID address, LPCVOID buffer, SIZE_T size) {
        SIZE_T bytes_written = 0;

        return WriteProcessMemory(process, address, buffer, size, &bytes_written)
            ? bytes_written
            : throw util::exception(
                "Failed to write process memory at address: '{}'", fmt::ptr(address)
            );
    }

    SIZE_T write_process_memory(const HANDLE& process, LPVOID address, LPCVOID buffer, SIZE_T size) {
        PANIC_ON_CATCH(write_process_memory, process, address, buffer, size)
    }

}
