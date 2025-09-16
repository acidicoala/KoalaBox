#include "pch.hpp"
#include "koalabox/globals.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/util.hpp"

namespace koalabox::util {
    void error_box(const std::string& title, const std::string& message) {
        ::MessageBox(
            nullptr,
            str::to_wstr(message).c_str(),
            str::to_wstr(title).c_str(),
            MB_OK | MB_ICONERROR
        );
    }

    [[noreturn]] void panic(const std::string& message) {
        const auto title = std::format("[{}] Panic!", globals::get_project_name());

        auto extended_message = message;

        OutputDebugString(str::to_wstr(message).c_str());
        LOG_CRITICAL("{}", extended_message);

        const int last_error = static_cast<int>(GetLastError());
        if(last_error != 0) {
            extended_message += std::format(
                "\n———————— Windows Last Error ————————\nCode: {}\nMessage: {}",
                last_error,
                win::format_message(last_error)
            );
        }

        error_box(title, extended_message);

        logger::shutdown();
        exit(last_error);
    }

    std::string get_env_var(const std::string& key) {
        const auto wide_key = str::to_wstr(key);

        // @doc https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getenvironmentvariable
        TCHAR buffer[32 * 1024];
        const auto bytes_read = GetEnvironmentVariable(wide_key.c_str(), buffer, sizeof(buffer));

        return str::to_str({buffer, bytes_read});
    }

    bool is_wine_env() {
        static const auto ntdll_handle = GetModuleHandle("ntdll.dll");
        if(!ntdll_handle) {
            // This should never happen
            DebugBreak();
            return false;
        }

        static const auto wine_function = GetProcAddress(ntdll_handle, "wine_get_version");

        return wine_function != nullptr;
    }
}
