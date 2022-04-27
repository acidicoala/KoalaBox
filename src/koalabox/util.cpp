#include <koalabox/util.hpp>
#include <koalabox/win_util.hpp>

namespace koalabox::util {

    void error_box(const String& title, const String& message) {
        ::MessageBoxW(nullptr, to_wstring(message).c_str(), to_wstring(title).c_str(), MB_OK | MB_ICONERROR);
    }

    bool is_x64() {
        // This is implemented as a memoized lazy function to avoid
        // "expression always true/false" warnings
        static const auto result = sizeof(void*) == 8;
        return result;
    }

    [[noreturn]] void panic(String message) {
        const auto title = fmt::format("[{}] Panic!", project_name);

        const auto last_error = ::GetLastError();
        if (last_error != 0) {
            message = fmt::format(
                "{}\n———————— Windows Last Error ————————\nCode: {}\nMessage: {}",
                message, last_error, win_util::format_message(last_error)
            );
        }

        logger->critical(message);

        error_box(title, message);

        exit(static_cast<int>(last_error));
    }

    bool strings_are_equal(const String& string1, const String& string2) {
        return _stricmp(string1.c_str(), string2.c_str()) == 0;
    }

    String to_string(const WideString& wstr) {
        if (wstr.empty()) {
            return {};
        }

        const auto required_size = WideCharToMultiByte(
            CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr
        );

        String string(required_size, 0);
        WideCharToMultiByte(
            CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), &string[0], required_size, nullptr, nullptr
        );

        return string;
    }

    WideString to_wstring(const String& str) {
        if (str.empty()) {
            return {};
        }

        const auto required_size = MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), nullptr, 0);

        WideString wstring(required_size, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), &wstring[0], required_size);

        return wstring;
    }

}
