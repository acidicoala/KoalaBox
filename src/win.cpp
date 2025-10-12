#include <utf8.h>
#include <wil/stl.h>
#include <wil/win32_helpers.h>

#include "koalabox/win.hpp"

#include "koalabox/core.hpp"
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

    std::optional<std::vector<uint8_t>> get_module_version_info(const HMODULE module_handle) noexcept {
        const auto module_path = lib::get_fs_path(module_handle);
        const auto module_path_wstr = path::to_wstr(module_path);

        DWORD version_handle = 0;
        if(const DWORD version_size = GetFileVersionInfoSize(module_path_wstr.c_str(), &version_handle)) {
            std::vector<uint8_t> version_data(version_size);

            if(not GetFileVersionInfo(module_path_wstr.c_str(), version_handle, version_size, version_data.data())) {
                return version_data;
            }
        }

        return std::nullopt;
    }

    std::vector<uint8_t> get_module_version_info_or_throw(const HMODULE module_handle) {
        return get_module_version_info(module_handle) | throw_if_empty(std::format("Unexpected empty version info"));
    }

    std::optional<std::string> get_version_info_string(
        const HMODULE module_handle,
        const std::string& key,
        const std::string& codepage
    ) noexcept {
        try {
            if(const auto version_info = get_module_version_info(module_handle)) {
                UINT size = 0;
                PWSTR product_name_wstr_ptr = nullptr;
                if(
                    VerQueryValue(
                        version_info->data(),
                        str::to_wstr(std::format(R"(\StringFileInfo\{}\{})", codepage, key)).c_str(),
                        reinterpret_cast<void**>(&product_name_wstr_ptr),
                        &size
                    )
                ) {
                    // returning {product_name_wstr_ptr, size} includes null-terminator char,
                    // which breaks string comparisons
                    return str::to_str(product_name_wstr_ptr);
                }
            }
            return std::nullopt;
        } catch(const std::exception& e) {
            LOG_ERROR("Failed to get version info string: {}", e.what());
            return std::nullopt;
        }
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

    std::string get_module_manifest(void* module_handle) {
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

            const HRSRC resource_handle = ::FindResource(hModule, lpName, lpType);
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
            static_cast<HMODULE>(module_handle),
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
