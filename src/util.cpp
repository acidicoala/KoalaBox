#include "koalabox/util.hpp"
#include "koalabox/globals.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/str.hpp"
#include "koalabox/win.hpp"

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

        OutputDebugString(str::to_wstr(message).c_str());

        auto extended_message = message;

        const auto last_error = GetLastError();
        if(last_error != 0) {
            extended_message += std::format(
                "\n———————— Windows Last Error ————————\nCode: {}\nMessage: {}",
                last_error,
                win::format_message(last_error)
            );
        }

        LOG_CRITICAL("{}", extended_message);

        error_box(title, extended_message);

        logger::shutdown();
        exit(static_cast<int>(last_error));
    }

    // Source: https://guidedhacking.com/threads/testing-if-pointer-is-invalid.13222/post-77709
    bool is_valid_pointer(const void* pointer) {
        if(const auto mbi_opt = win::virtual_query(pointer)) {
            const auto is_rwe =
                mbi_opt->Protect &
                (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ |
                 PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);

            const auto is_guarded = mbi_opt->Protect & (PAGE_GUARD | PAGE_NOACCESS);

            return is_rwe && !is_guarded;
        }

        return false;
    }
}
