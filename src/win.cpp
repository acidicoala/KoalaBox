#include <utf8.h>
#include <wil/stl.h>
#include <wil/win32_helpers.h>

#include "koalabox/win.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/path.hpp"
#include "koalabox/str.hpp"
#include "koalabox/util.hpp"

#pragma comment(lib, "version.lib")

/**
 * NOTE: It's important not to log anything in these functions, since logging might not have been
 * initialized yet. All errors must be reported via exceptions, which will be then displayed to user
 * via logs or message boxes depending on the circumstance.
 */
namespace koalabox::win {
    namespace {
        std::vector<uint8_t> get_module_version_info_or_throw(const HMODULE& module_handle) {
            const auto module_path = get_module_path(module_handle);
            const auto module_path_wstr = path::to_wstr(module_path);

            DWORD version_handle = 0;
            const DWORD version_size = GetFileVersionInfoSize(
                module_path_wstr.c_str(),
                &version_handle
            );

            if(not version_size) {
                throw std::runtime_error(
                    std::format(
                        "Failed to GetFileVersionInfoSize. Error: {}",
                        get_last_error()
                    )
                );
            }

            std::vector<uint8_t> version_data(version_size);

            if(not GetFileVersionInfo(
                module_path_wstr.c_str(),
                version_handle,
                version_size,
                version_data.data()
            )) {
                throw std::runtime_error(
                    std::format("Failed to GetFileVersionInfo. Error: {}", get_last_error())
                );
            }

            return version_data;
        }
    }

#define PANIC_ON_CATCH(FUNC, ...)                                                                  \
    try {                                                                                          \
        return FUNC(__VA_ARGS__);                                                                  \
    } catch (const std::exception& ex) {                                                           \
        util::panic(ex.what());                                                                    \
    }

    PROCESS_INFORMATION create_process(
        const std::string& app_name,
        const std::string& args,
        const fs::path& working_dir,
        const bool show_window
    ) {
        const auto working_dir_wstr = path::to_wstr(working_dir);

        PROCESS_INFORMATION process_info{};
        STARTUPINFO startup_info{};

        const auto cmd_line = app_name + " " + args;

        LOG_TRACE("Launching {}", cmd_line);

        const auto success = CreateProcess(
                                 nullptr,
                                 str::to_wstr(cmd_line).data(),
                                 nullptr,
                                 nullptr,
                                 NULL,
                                 show_window ? 0 : CREATE_NO_WINDOW,
                                 nullptr,
                                 working_dir_wstr.c_str(),
                                 &startup_info,
                                 &process_info
                             ) != 0;

        if(!success) {
            DWORD exit_code = 0;
            GetExitCodeProcess(process_info.hProcess, &exit_code);

            throw std::runtime_error(
                std::format(
                    R"(Error creating process "{}" with args "{}" at "{}". Last error: {}, Exit code: {})",
                    app_name,
                    args,
                    path::to_str(working_dir),
                    get_last_error(),
                    exit_code
                )
            );
        }

        WaitForInputIdle(process_info.hProcess, INFINITE);

        return process_info;
    }

    std::string format_message(const DWORD message_id) {
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

    std::string get_last_error() {
        return std::format("0x{0:x}", GetLastError());
    }

    fs::path get_module_path(const HMODULE& handle) {
        const auto wstr_path = wil::GetModuleFileNameW<std::wstring>(handle);
        return path::from_wstr(wstr_path);
    }

    HMODULE get_module_handle_or_throw(LPCSTR module_name) {
        auto* const handle = module_name
                                 ? ::GetModuleHandle(str::to_wstr(module_name).c_str())
                                 : ::GetModuleHandle(nullptr);

        return handle
                   ? handle
                   : throw std::runtime_error(
                       std::format("Failed to get a handle of the module: '{}'", module_name)
                   );
    }

    HMODULE get_module_handle(LPCSTR module_name) {
        PANIC_ON_CATCH(get_module_handle_or_throw, module_name)
    }

    MODULEINFO get_module_info_or_throw(const HMODULE& module_handle) {
        MODULEINFO module_info = {nullptr};
        const auto success = ::GetModuleInformation(
            GetCurrentProcess(),
            module_handle,
            &module_info,
            sizeof(module_info)
        );

        return success
                   ? module_info
                   : throw std::runtime_error(
                       std::format(
                           "Failed to get module info of the given module handle: {}",
                           fmt::ptr(module_handle)
                       )
                   );
    }

    MODULEINFO get_module_info(const HMODULE& module_handle) {
        PANIC_ON_CATCH(get_module_info_or_throw, module_handle)
    }

    std::string get_module_manifest(const HMODULE& module_handle) {
        struct Response {
            bool success = false;
            std::string manifest_or_error;
        };

        // Return TRUE to continue enumeration or FALSE to stop enumeration.
        const auto callback = [](
            const HMODULE hModule,
            const LPCTSTR lpType,
            const LPTSTR lpName,
            const LONG_PTR lParam
        ) -> BOOL {
            const auto response = [&](const bool success, std::string manifest_or_error) {
                try {
                    auto* res = reinterpret_cast<Response*>(lParam);

                    if(!success) {
                        manifest_or_error = fmt::format(
                            "get_module_manifest_callback -> {}. Last error: {}",
                            manifest_or_error,
                            get_last_error()
                        );
                    }

                    res->success = success;
                    res->manifest_or_error = manifest_or_error;

                    return TRUE;
                } catch(const std::exception& e) {
                    LOG_ERROR("EnumResourceNames callback error: {}", e.what());

                    return FALSE;
                }
            };

            HRSRC resource_handle = ::FindResource(hModule, lpName, lpType);
            if(resource_handle == nullptr) {
                return response(false, "FindResource returned null.");
            }

            const auto resource_size = SizeofResource(hModule, resource_handle);
            if(resource_size == 0) {
                return response(false, "SizeofResource returned 0.");
            }

            HGLOBAL resource_data_handle = LoadResource(hModule, resource_handle);
            if(resource_data_handle == nullptr) {
                return response(false, "LockResource returned null.");
            }

            const auto* resource_data = LockResource(resource_data_handle);
            if(resource_data == nullptr) {
                return response(false, "Resource data is null.");
            }

            return response(true, static_cast<const char*>(resource_data));
        };

        Response response;
        if(not EnumResourceNames(
            module_handle,
            RT_MANIFEST,
            callback,
            reinterpret_cast<LONG_PTR>(&response)
        )) {
            throw std::runtime_error(
                std::format("EnumResourceNames call error. Last error: {}", get_last_error())
            );
        }

        if(!response.success) {
            throw std::runtime_error(response.manifest_or_error);
        }

        return response.manifest_or_error;
    }

    std::string get_module_version_or_throw(const HMODULE& module_handle) {
        const auto version_data = get_module_version_info_or_throw(module_handle);

        UINT size = 0;
        VS_FIXEDFILEINFO* version_info = nullptr;
        if(not VerQueryValue(
                version_data.data(),
                TEXT("\\"),
                reinterpret_cast<void* *>(&version_info),
                &size
            )
        ) {
            throw std::runtime_error(
                std::format("Failed to VerQueryValue. Error: {}", get_last_error())
            );
        }

        if(not size) {
            throw std::runtime_error(
                std::format("Failed to VerQueryValue. Error: {}", get_last_error())
            );
        }

        if(version_info->dwSignature != 0xfeef04bd) {
            throw std::runtime_error(
                std::format(
                    "VerQueryValue signature mismatch. Signature: 0x{0:x}",
                    version_info->dwSignature
                )
            );
        }

        return fmt::format(
            "{}.{}.{}.{}",
            (version_info->dwFileVersionMS >> 16) & 0xffff,
            (version_info->dwFileVersionMS >> 0) & 0xffff,
            (version_info->dwFileVersionLS >> 16) & 0xffff,
            (version_info->dwFileVersionLS >> 0) & 0xffff
        );
    }

    PIMAGE_SECTION_HEADER get_pe_section_or_throw(
        const HMODULE& module_handle,
        const std::string& section_name
    ) {
        auto* const dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);

        if(dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
            throw std::runtime_error("Invalid DOS file");
        }

        auto* const nt_header =
            reinterpret_cast<PIMAGE_NT_HEADERS>(
                reinterpret_cast<uint8_t*>(module_handle) + dos_header->e_lfanew);

        if(nt_header->Signature != IMAGE_NT_SIGNATURE) {
            throw std::runtime_error("Invalid NT signature");
        }

        auto* section = IMAGE_FIRST_SECTION(nt_header);
        for(int i = 0; i < nt_header->FileHeader.NumberOfSections; i++, section++) {
            auto name = std::string(reinterpret_cast<char*>(section->Name), 8);
            name = name.substr(0, name.find('\0')); // strip null padding

            if(name != section_name) {
                continue;
            }

            return section;
        }

        throw std::runtime_error(std::format("Section '{}' not found", section_name));
    }

    std::string get_pe_section_data_or_throw(
        const HMODULE& module_handle,
        const std::string& section_name
    ) {
        const auto* section = get_pe_section_or_throw(module_handle, section_name);
        return {
            reinterpret_cast<char*>(module_handle) + section->PointerToRawData,
            section->SizeOfRawData
        };
    }

    std::string get_pe_section_data(const HMODULE& module_handle, const std::string& section_name) {
        PANIC_ON_CATCH(get_pe_section_data_or_throw, module_handle, section_name)
    }

    FARPROC get_proc_address_or_throw(const HMODULE& handle, LPCSTR procedure_name) {
        const auto address = GetProcAddress(handle, procedure_name);

        return address
                   ? address
                   : throw std::runtime_error(
                       std::format(
                           "Failed to get the address of the procedure: '{}'",
                           procedure_name
                       )
                   );
    }

    FARPROC get_proc_address(const HMODULE& handle, LPCSTR procedure_name) {
        PANIC_ON_CATCH(get_proc_address_or_throw, handle, procedure_name)
    }

    fs::path get_system_directory_or_throw() {
        WCHAR path[MAX_PATH];
        const auto result = GetSystemDirectory(path, MAX_PATH);

        if(result > MAX_PATH) {
            throw std::runtime_error(
                std::format(
                    "GetSystemDirectory path length ({}) is greater than MAX_PATH ({})",
                    result,
                    MAX_PATH
                )
            );
        }

        if(result == 0) {
            throw std::exception("Failed to get the path of the system directory");
        }

        return std::filesystem::absolute(path);
    }

    fs::path get_system_directory() {
        PANIC_ON_CATCH(get_system_directory_or_throw)
    }

    void free_library_or_throw(const HMODULE& module_handle) {
        if(not FreeLibrary(module_handle)) {
            throw std::runtime_error(
                std::format(
                    "Failed to free a library with the given module handle: {}",
                    static_cast<void*>(module_handle)
                )
            );
        }
    }

    bool free_library(const HMODULE& handle, bool panic_on_fail) {
        try {
            free_library_or_throw(handle);
            return true;
        } catch(std::exception& ex) {
            if(panic_on_fail) {
                util::panic(ex.what());
            }

            return false;
        }
    }

    HMODULE load_library_or_throw(const fs::path& module_path) {
        const auto module_path_wstr = path::to_wstr(module_path);
        auto* const module_handle = LoadLibrary(module_path_wstr.c_str());

        return module_handle
                   ? module_handle
                   : throw std::runtime_error(
                       std::format(
                           "Failed to load the module at path: '{}'",
                           str::to_str(module_path_wstr)
                       )
                   );
    }

    HMODULE load_library(const fs::path& module_path) {
        PANIC_ON_CATCH(load_library_or_throw, module_path)
    }

    void register_application_restart() {
        if(const auto result = RegisterApplicationRestart(nullptr, 0); result != S_OK) {
            throw std::runtime_error(
                std::format("Failed to register application restart. Result: {}", result)
            );
        }
    }

    std::optional<MEMORY_BASIC_INFORMATION> virtual_query(const void* pointer) {
        MEMORY_BASIC_INFORMATION mbi{};

        if(VirtualQuery(pointer, &mbi, sizeof(mbi))) {
            return mbi;
        }

        return {};
    }

    SIZE_T write_process_memory_or_throw(
        const HANDLE& process,
        const LPVOID address,
        const LPCVOID buffer,
        const SIZE_T size
    ) {
        SIZE_T bytes_written = 0;

        return WriteProcessMemory(process, address, buffer, size, &bytes_written)
                   ? bytes_written
                   : throw std::runtime_error(
                       std::format(
                           "Failed to write process memory at address: '{}'",
                           fmt::ptr(address)
                       )
                   );
    }

    SIZE_T write_process_memory(
        const HANDLE& process,
        const LPVOID address,
        const LPCVOID buffer,
        const SIZE_T size
    ) {
        PANIC_ON_CATCH(write_process_memory_or_throw, process, address, buffer, size)
    }

    std::string get_env_var(const std::string& key) {
        const auto wide_key = str::to_wstr(key);

        // @doc https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getenvironmentvariable
        TCHAR buffer[32 * 1024];
        const auto bytes_read = GetEnvironmentVariable(wide_key.c_str(), buffer, sizeof(buffer));

        return str::to_str({buffer, bytes_read});
    }
}
