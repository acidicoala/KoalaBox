#include <koalabox/win_util.hpp>
#include <koalabox/util.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/str.hpp>

#pragma comment(lib, "Version.lib")

/**
 * NOTE: It's important not to log anything in these functions, since logging might not have been
 * initialized yet. All errors must be reported via exceptions, which will be then displayed to user
 * via logs or message boxes depending on the circumstance.
 */
namespace koalabox::win_util {

    namespace {
        Vector<uint8_t> get_file_version_info_or_throw(const HMODULE& module_handle) {
            const auto file_name = str::to_wstr(get_module_file_name_or_throw(module_handle));

            DWORD version_handle = 0;
            const DWORD version_size = GetFileVersionInfoSize(file_name.c_str(), &version_handle);

            if (not version_size) {
                throw util::exception("Failed to GetFileVersionInfoSize. Error: {}",
                    get_last_error());
            }

            Vector<uint8_t> version_data(version_size);

            if (not GetFileVersionInfo(file_name.c_str(), version_handle, version_size,
                version_data.data())) {
                throw util::exception("Failed to GetFileVersionInfo. Error: {}", get_last_error());
            }

            return version_data;
        }
    }

#define PANIC_ON_CATCH(FUNC, ...) \
    try { \
        return FUNC##_or_throw(__VA_ARGS__); \
    } catch (const Exception& ex) { \
        util::panic(ex.what()); \
    }

    KOALABOX_API(PROCESS_INFORMATION) create_process(
        const String& app_name,
        const String& args,
        const Path& working_dir,
        bool show_window
    ) {
        DECLARE_STRUCT(PROCESS_INFORMATION, process_info);
        DECLARE_STRUCT(STARTUPINFO, startup_info);

        const auto cmd_line = app_name + " " + args;

        LOG_TRACE("Launching {}",cmd_line);

        const auto success = CreateProcess(
            NULL,
            str::to_wstr(cmd_line).data(),
            NULL,
            NULL,
            NULL,
            show_window ? 0 : CREATE_NO_WINDOW,
            NULL,
            working_dir.wstring().c_str(),
            &startup_info,
            &process_info
        ) != 0;

        if (!success) {
            DWORD exit_code = 0;
            GetExitCodeProcess(process_info.hProcess, &exit_code);

            throw util::exception(
                R"(Error creating process "{}" with args "{}" at "{}". Last error: {}, Exit code: {})",
                app_name, args, working_dir.string(), get_last_error(), exit_code
            );
        }

        WaitForInputIdle(process_info.hProcess, INFINITE);

        return process_info;
    }

    KOALABOX_API(String) format_message(const DWORD message_id) {
        TCHAR buffer[1024];

        ::FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            message_id,
            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
            buffer,
            sizeof(buffer) / sizeof(TCHAR),
            nullptr
        );

        return str::to_str(buffer);
    }

    KOALABOX_API(String) get_last_error() {
        return fmt::format("0x{0:x}", ::GetLastError());
    }

    KOALABOX_API(String) get_module_file_name_or_throw(const HMODULE& module_handle) {
        constexpr auto buffer_size = 1024;
        TCHAR buffer[buffer_size];
        const auto length = ::GetModuleFileName(module_handle, buffer, buffer_size);

        return (length > 0 and length < buffer_size)
            ? str::to_str(buffer)
            : throw util::exception(
                "Failed to get a file name of the given module handle: {}. Length: {}",
                fmt::ptr(module_handle), length
            );
    }

    KOALABOX_API(String) get_module_file_name(const HMODULE& module_handle) {
        PANIC_ON_CATCH(get_module_file_name, module_handle)
    }

    KOALABOX_API(HMODULE) get_module_handle_or_throw(LPCSTR module_name) {
        auto* const handle = module_name
            ? ::GetModuleHandle(str::to_wstr(module_name).c_str())
            : ::GetModuleHandle(nullptr);

        return handle ? handle : throw util::exception(
            "Failed to get a handle of the module: '{}'", module_name
        );
    }

    KOALABOX_API(HMODULE) get_module_handle(LPCSTR module_name) {
        PANIC_ON_CATCH(get_module_handle, module_name)
    }

    KOALABOX_API(MODULEINFO) get_module_info_or_throw(const HMODULE& module_handle) {
        MODULEINFO module_info = { nullptr };
        const auto success = ::GetModuleInformation(
            GetCurrentProcess(), module_handle, &module_info, sizeof(module_info)
        );

        return success ? module_info : throw util::exception(
            "Failed to get module info of the given module handle: {}", fmt::ptr(module_handle)
        );
    }

    KOALABOX_API(MODULEINFO) get_module_info(const HMODULE& module_handle) {
        PANIC_ON_CATCH(get_module_info, module_handle)
    }

    KOALABOX_API(String) get_module_manifest(const HMODULE& module_handle) {
        struct Response {
            bool success = false;
            String manifest_or_error;
        };

        // Return TRUE to continue enumeration or FALSE to stop enumeration.
        const auto callback = [](
            HMODULE hModule,
            LPCTSTR lpType,
            LPTSTR lpName,
            LONG_PTR lParam
        ) -> BOOL {
            const auto response = [&](bool success, String manifest_or_error) {
                try {
                    auto* response = (Response*) lParam;

                    if (!success) {
                        manifest_or_error = fmt::format(
                            "get_module_manifest_callback -> {}. Last error: {}", manifest_or_error,
                            get_last_error()
                        );
                    }

                    response->success = success;
                    response->manifest_or_error = manifest_or_error;

                    return TRUE;
                } catch (const Exception& e) {
                    LOG_ERROR("EnumResourceNames callback error: {}", e.what());

                    return FALSE;
                }
            };

            HRSRC resource_handle = ::FindResource(hModule, lpName, lpType);
            if (resource_handle == nullptr) {
                return response(false, "FindResource returned null.");
            }

            const auto resource_size = SizeofResource(hModule, resource_handle);
            if (resource_size == 0) {
                return response(false, "SizeofResource returned 0.");
            }

            HGLOBAL resource_data_handle = LoadResource(hModule, resource_handle);
            if (resource_data_handle == nullptr) {
                return response(false, "LockResource returned null.");
            }

            const auto* resource_data = LockResource(resource_data_handle);
            if (resource_data == nullptr) {
                return response(false, "Resource data is null.");
            }

            return response(true, (char*) resource_data);
        };

        Response response;
        if (not EnumResourceNames(module_handle, RT_MANIFEST, callback, (LONG_PTR) &response)) {
            throw util::exception("EnumResourceNames call error. Last error: {}", get_last_error());
        }

        if (!response.success) {
            throw util::exception("{}", response.manifest_or_error);
        }

        return response.manifest_or_error;
    }

    KOALABOX_API(String) get_module_version_or_throw(const HMODULE& module_handle) {
        const auto version_data = get_file_version_info_or_throw(module_handle);

        UINT size = 0;
        VS_FIXEDFILEINFO* version_info = nullptr;
        if (not VerQueryValue(version_data.data(), TEXT("\\"), (VOID FAR* FAR*) &version_info,
            &size)) {
            throw util::exception("Failed to VerQueryValue. Error: {}", get_last_error());
        }

        if (not size) {
            throw util::exception("Failed to VerQueryValue. Error: {}", get_last_error());
        }

        if (version_info->dwSignature != 0xfeef04bd) {
            throw util::exception("VerQueryValue signature mismatch. Signature: 0x{0:x}",
                version_info->dwSignature);
        }

        return fmt::format(
            "{}.{}.{}.{}",
            (version_info->dwFileVersionMS >> 16) & 0xffff,
            (version_info->dwFileVersionMS >> 0) & 0xffff,
            (version_info->dwFileVersionLS >> 16) & 0xffff,
            (version_info->dwFileVersionLS >> 0) & 0xffff
        );
    }

    KOALABOX_API(String) get_pe_section_data_or_throw(
        const HMODULE& module_handle, const String& section_name
    ) {
        auto* const dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);

        if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
            throw util::exception("Invalid DOS file");
        }

        auto* const nt_header = (PIMAGE_NT_HEADERS) ((uint8_t*) module_handle +
                                                     (dos_header->e_lfanew));

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

            return { (char*) module_handle + section->PointerToRawData, section->SizeOfRawData };
        }

        throw util::exception("Section '{}' not found", section_name);
    }

    KOALABOX_API(String) get_pe_section_data(
        const HMODULE& module_handle, const String& section_name
    ) {
        PANIC_ON_CATCH(get_pe_section_data, module_handle, section_name)
    }

    KOALABOX_API(FARPROC) get_proc_address_or_throw(const HMODULE& handle, LPCSTR procedure_name) {
        const auto address = ::GetProcAddress(handle, procedure_name);

        return address
            ? address
            : throw util::exception("Failed to get the address of the procedure: '{}'",
                procedure_name);
    }

    KOALABOX_API(FARPROC) get_proc_address(const HMODULE& handle, LPCSTR procedure_name) {
        PANIC_ON_CATCH(get_proc_address, handle, procedure_name)
    }

    KOALABOX_API(Path) get_system_directory_or_throw() {
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

    KOALABOX_API(Path) get_system_directory() {
        PANIC_ON_CATCH(get_system_directory)
    }

    KOALABOX_API(void) free_library_or_throw(const HMODULE& handle) {
        if (not::FreeLibrary(handle)) {
            throw util::exception("Failed to free a library with the given module handle: {}",
                fmt::ptr(handle));
        }
    }

    KOALABOX_API(bool) free_library(const HMODULE& handle, bool panic_on_fail) {
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

    KOALABOX_API(HMODULE) load_library_or_throw(const Path& module_path) {
        auto* const module_handle = ::LoadLibrary(module_path.wstring().c_str());

        return module_handle
            ? module_handle
            : throw util::exception("Failed to load the module at path: '{}'",
                module_path.string());
    }

    KOALABOX_API(HMODULE) load_library(const Path& module_path) {
        PANIC_ON_CATCH(load_library, module_path)
    }

    KOALABOX_API(void) register_application_restart() {
        const auto result = RegisterApplicationRestart(nullptr, 0);

        if (result != S_OK) {
            throw util::exception("Failed to register application restart. Result: {}", result);
        }
    }

    KOALABOX_API(std::optional<MEMORY_BASIC_INFORMATION>) virtual_query(const void* pointer) {
        MEMORY_BASIC_INFORMATION mbi{};

        if (::VirtualQuery(pointer, &mbi, sizeof(mbi))) {
            return mbi;
        }

        return std::nullopt;
    }

    KOALABOX_API(SIZE_T) write_process_memory_or_throw(
        const HANDLE& process, LPVOID address, LPCVOID buffer, SIZE_T size
    ) {
        SIZE_T bytes_written = 0;

        return WriteProcessMemory(process, address, buffer, size, &bytes_written)
            ? bytes_written
            : throw util::exception(
                "Failed to write process memory at address: '{}'", fmt::ptr(address)
            );
    }

    KOALABOX_API(SIZE_T) write_process_memory(
        const HANDLE& process, LPVOID address, LPCVOID buffer, SIZE_T size
    ) {
        PANIC_ON_CATCH(write_process_memory, process, address, buffer, size)
    }

}
