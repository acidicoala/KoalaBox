#include "koalabox/util.hpp"
#include "koalabox/globals.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/str.hpp"
#include "koalabox/win_util.hpp"

namespace koalabox::util {

    void error_box(const std::string& title, const std::string& message) {
        ::MessageBox(
            nullptr, str::to_wstr(message).c_str(), str::to_wstr(title).c_str(),
            MB_OK | MB_ICONERROR
        );
    }

    [[noreturn]] void panic(std::string message) {
        const auto title = std::format("[{}] Panic!", globals::get_project_name());

        OutputDebugString(str::to_wstr(message).c_str());

        const auto last_error = ::GetLastError();
        if (last_error != 0) {
            message += std::format(
                "\n———————— Windows Last Error ————————\nCode: {}\nMessage: {}", last_error,
                win_util::format_message(last_error)
            );
        }

        LOG_CRITICAL("{}", message);

        error_box(title, message);

        logger::shutdown();
        exit(static_cast<int>(last_error));
    }

    // Source: https://guidedhacking.com/threads/testing-if-pointer-is-invalid.13222/post-77709
    bool is_valid_pointer(const void* pointer) {
        const auto mbi_opt = win_util::virtual_query(pointer);

        if (mbi_opt) {
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
