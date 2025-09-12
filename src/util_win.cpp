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
}
