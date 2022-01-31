#include "util.hpp"
#include "logger/logger.hpp"
#include "win_util/win_util.hpp"
#include "loader/loader.hpp"
#include <Windows.h>

using namespace koalabox;

[[maybe_unused]]
void util::error_box(String& title, String& message) {
    ::MessageBoxW(
        nullptr,
        util::to_wstring(message).c_str(),
        util::to_wstring(title).c_str(),
        MB_OK | MB_ICONERROR
    );
}

[[maybe_unused]]
Path util::get_working_dir() {
    static auto working_dir = [] {
        auto this_module_name = loader::get_this_module_name();

        auto handle = win_util::get_module_handle(this_module_name);

        auto file_name = win_util::get_module_file_name(handle);

        auto file_path = std::filesystem::path(file_name);

        return file_path.parent_path();
    }();

    return working_dir;
}

[[maybe_unused]]
bool util::is_64_bit() {
    // Static variables are used to suppress
    // "Condition always true/false" warnings
#ifdef _WIN64
    return true;
#else
    return false;
#endif
}

[[maybe_unused]]
void util::panic(String title, String message) {
    // Add windows last error to title and message, if necessary.
    auto last_error = ::GetLastError();
    if (last_error != 0) {
        message = fmt::format(
            "{}\n———————— Windows Last Error ————————\nCode: {}\nMessage: {}",
            message,
            last_error,
            win_util::format_message(last_error)
        );
    }

    error_box(title, message);

    logger::critical("{}", message);

    exit(static_cast<int>(last_error));
}

[[maybe_unused]]
std::string util::to_string(const std::wstring& wstr) {
    if (wstr.empty()) {
        return {};
    }
    int size_needed = WideCharToMultiByte(
        CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr
    );
    std::string string(size_needed, 0);
    WideCharToMultiByte(
        CP_UTF8, 0, &wstr[0], (int) wstr.size(), &string[0], size_needed, nullptr, nullptr
    );
    return string;
}

[[maybe_unused]]
std::wstring util::to_wstring(const std::string& str) {
    if (str.empty()) {
        return {};
    }
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int) str.size(), nullptr, 0);
    std::wstring wstring(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int) str.size(), &wstring[0], size_needed);
    return wstring;
}
