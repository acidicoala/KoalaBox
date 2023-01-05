#include <koalabox/win_util.hpp>
#include <koalabox/util.hpp>

#pragma comment(lib, "Version.lib")

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

    String get_last_error() {
        const auto error = ::GetLastError();
        return fmt::format("0x{0:x}", error);
    }

    String get_module_file_name_or_throw(const HMODULE& module_handle) {
        constexpr auto buffer_size = 1024;
        TCHAR buffer[buffer_size];
        const auto length = ::GetModuleFileName(module_handle, buffer, buffer_size);

        return (length > 0 and length < buffer_size)
               ? util::to_string(buffer)
               : throw util::exception(
                "Failed to get a file name of the given module handle: {}. Length: {}",
                fmt::ptr(module_handle), length
            );
    }

    String get_module_file_name(const HMODULE& module_handle) {
        PANIC_ON_CATCH(get_module_file_name, module_handle)
    }

    HMODULE get_module_handle_or_throw(LPCSTR module_name) {
        auto* const handle = module_name
                             ? ::GetModuleHandle(util::to_wstring(module_name).c_str())
                             : ::GetModuleHandle(nullptr);

        return handle ? handle : throw util::exception(
            "Failed to get a handle of the module: '{}'", module_name
        );
    }

    HMODULE get_module_handle(LPCSTR module_name) {
        PANIC_ON_CATCH(get_module_handle, module_name)
    }

    [[maybe_unused]] MODULEINFO get_module_info_or_throw(const HMODULE& module_handle) {
        MODULEINFO module_info = {nullptr};
        const auto success = ::GetModuleInformation(
            GetCurrentProcess(), module_handle, &module_info, sizeof(module_info)
        );

        return success ? module_info : throw util::exception(
            "Failed to get module info of the given module handle: {}", fmt::ptr(module_handle)
        );
    }

    MODULEINFO get_module_info(const HMODULE& module_handle) {
        PANIC_ON_CATCH(get_module_info, module_handle)
    }

    String get_module_version_or_throw(const HMODULE& module_handle) {
        const auto file_name = util::to_wstring(get_module_file_name_or_throw(module_handle));

        DWORD version_handle = 0;
        const DWORD version_size = GetFileVersionInfoSize(file_name.c_str(), &version_handle);

        if (not version_size) {
            throw util::exception("Failed to GetFileVersionInfoSize. Error: {}", get_last_error());
        }

        Vector<uint8_t> version_data(version_size);

        if (not GetFileVersionInfo(file_name.c_str(), version_handle, version_size, version_data.data())) {
            throw util::exception("Failed to GetFileVersionInfo. Error: {}", get_last_error());
        }

        UINT size = 0;
        VS_FIXEDFILEINFO* version_info = nullptr;
        if (not VerQueryValue(version_data.data(), TEXT("\\"), (VOID FAR* FAR*) &version_info, &size)) {
            throw util::exception("Failed to VerQueryValue. Error: {}", get_last_error());
        }

        if (not size) {
            throw util::exception("Failed to VerQueryValue. Error: {}", get_last_error());
        }

        if (version_info->dwSignature != 0xfeef04bd) {
            throw util::exception("VerQueryValue signature mismatch. Signature: 0x{0:x}", version_info->dwSignature);
        }

        return fmt::format("{}.{}.{}.{}",
                           (version_info->dwFileVersionMS >> 16) & 0xffff,
                           (version_info->dwFileVersionMS >> 0) & 0xffff,
                           (version_info->dwFileVersionLS >> 16) & 0xffff,
                           (version_info->dwFileVersionLS >> 0) & 0xffff
        );
    }

    String get_pe_section_data_or_throw(const HMODULE& module_handle, const String& section_name) {
        auto* const dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);

        if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
            throw util::exception("Invalid DOS file");
        }

        auto* const nt_header = (PIMAGE_NT_HEADERS) ((uint8_t*) module_handle + (dos_header->e_lfanew));

        if (nt_header->Signature != IMAGE_NT_SIGNATURE) {
            throw util::exception("Invalid NT signature");
        }

        auto* section = IMAGE_FIRST_SECTION(nt_header);
        for (int i = 0; i < nt_header->FileHeader.NumberOfSections; i++, section++) {
            auto name = String((char*) section->Name, 8);
            name = name.substr(0, name.find('\0')); // strip null padding

            if (name != section_name) {
                continue;
            }

            return {(char*) module_handle + section->PointerToRawData, section->SizeOfRawData};
        }

        throw util::exception("Section '{}' not found", section_name);
    }

    String get_pe_section_data(const HMODULE& module_handle, const String& section_name) {
        PANIC_ON_CATCH(get_pe_section_data, module_handle, section_name)
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

    [[maybe_unused]] Path get_system_directory_or_throw() {
        WCHAR path[MAX_PATH];
        const auto result = GetSystemDirectory(path, MAX_PATH);

        if (result > MAX_PATH) {
            throw util::exception(
                "GetSystemDirectory path length ({}) is greater than MAX_PATH ({})",
                result, MAX_PATH
            );
        }

        if (result == 0) {
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
        auto* const module_handle = ::LoadLibrary(module_path.wstring().c_str());

        return module_handle
               ? module_handle
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
