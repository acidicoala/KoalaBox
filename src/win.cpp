#include <utf8.h>
#include <wil/stl.h>
#include <wil/win32_helpers.h>

#include "koalabox/win.hpp"

#include "koalabox/globals.hpp"
#include "koalabox/lib.hpp"
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
#define PANIC_ON_CATCH(FUNC, ...)                                                                  \
    try {                                                                                          \
        return FUNC(__VA_ARGS__);                                                                  \
    } catch (const std::exception& ex) {                                                           \
        util::panic(ex.what());                                                                    \
    }

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

    std::optional<std::string> get_version_info_string(
        const HMODULE module_handle,
        const std::string& key,
        const std::string& codepage
    ) noexcept {
        try {
            const auto version_info = get_module_version_info_or_throw(module_handle);

            UINT size = 0;
            PWSTR product_name_wstr_ptr = nullptr;
            const auto success = VerQueryValue(
                version_info.data(),
                str::to_wstr(std::format(R"(\StringFileInfo\{}\{})", codepage, key)).c_str(),
                reinterpret_cast<void**>(&product_name_wstr_ptr),
                &size
            );

            if(not success) {
                // LOG_TRACE("Failed to get version info string: VerQueryValue call returned false");
                return std::nullopt;
            }

            // returning {product_name_wstr_ptr, size} includes null-terminator char, which breaks string comparisons
            return str::to_str(product_name_wstr_ptr);
        } catch(const std::exception& e) {
            LOG_ERROR("Failed to get version info string: {}", e.what());
            return std::nullopt;
        }
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

    HMODULE get_module_handle_or_throw(const std::string& module_name) {
        auto* const handle = GetModuleHandle(str::to_wstr(module_name).c_str());

        if(not handle) {
            throw std::runtime_error(
                std::format("Failed to get a handle of the module: '{}'", module_name)
            );
        }

        return handle;
    }

    HMODULE get_module_handle(const std::string& module_name) {
        PANIC_ON_CATCH(get_module_handle_or_throw, module_name)
    }

    HMODULE get_process_handle() {
        return GetModuleHandle(nullptr);
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
                           reinterpret_cast<void*>(module_handle)
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
                        manifest_or_error = std::format(
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

        return std::format(
            "{}.{}.{}.{}",
            (version_info->dwFileVersionMS >> 16) & 0xffff,
            (version_info->dwFileVersionMS >> 0) & 0xffff,
            (version_info->dwFileVersionLS >> 16) & 0xffff,
            (version_info->dwFileVersionLS >> 0) & 0xffff
        );
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
                       std::format("Failed to write process memory at address: '{}'", address)
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

    void check_self_duplicates() noexcept {
        try {
            DWORD cbNeeded;
            HMODULE modules[1024];
            if(EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &cbNeeded)) {
                const auto* const self_handle = globals::get_self_handle();
                const auto& project_name = globals::get_project_name();

                const auto count = cbNeeded / sizeof(HMODULE);
                for(int i = 0; i < count; i++) {
                    if(const auto product_name = get_version_info_string(modules[i], "ProductName")) {
                        if(modules[i] != self_handle && *product_name == project_name) {
                            const auto module_path = lib::get_fs_path(modules[i]);
                            LOG_WARN(R"(Found a duplicate {} DLL: "{}")", project_name, path::to_str(module_path));
                            LOG_WARN(
                                "This will likely lead to errors or crashes. "
                                "Have you mixed up hook and proxy modes together?"
                            );
                        }
                    }
                }
            } else {
                LOG_ERROR("Failed to enumerated process modules. Last error: {}", GetLastError());
            }
        } catch(const std::exception& e) {
            LOG_ERROR("Failed to check self duplicates: {}", e.what());
        }
    }
}
