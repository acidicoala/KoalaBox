#include <koalabox/util.hpp>
#include <koalabox/win_util.hpp>
#include <koalabox/logger.hpp>

namespace koalabox::util {

    void error_box(const String& title, const String& message) {
        ::MessageBoxW(nullptr, to_wstring(message).c_str(), to_wstring(title).c_str(), MB_OK | MB_ICONERROR);
    }

    // TODO: Replace with macro
    bool is_x64() {
        // This is implemented as a memoized lazy function to avoid
        // "expression always true/false" warnings
        static const auto result = sizeof(uintptr_t) == 8;
        return result;
    }

    [[noreturn]] void panic(String message) {
        const auto last_error = ::GetLastError();
        if (last_error != 0) {
            message = fmt::format(
                "{}\n———————— Windows Last Error ————————\nCode: {}\nMessage: {}",
                message, last_error, win_util::format_message(last_error)
            );
        }

        LOG_CRITICAL(message)

        error_box("Panic!", message);

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
            CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr
        );

        String string(required_size, 0);
        WideCharToMultiByte(
            CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), string.data(), required_size, nullptr, nullptr
        );

        return string;
    }

    WideString to_wstring(const String& str) {
        if (str.empty()) {
            return {};
        }

        const auto required_size = MultiByteToWideChar(
            CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0
        );

        WideString wstring(required_size, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), wstring.data(), required_size);

        return wstring;
    }

    bool is_valid_pointer(const void* pointer) {
        MEMORY_BASIC_INFORMATION mbi = {nullptr};
        if (::VirtualQuery(pointer, &mbi, sizeof(mbi))) {
            const auto is_rwe = mbi.Protect & (
                PAGE_READONLY |
                PAGE_READWRITE |
                PAGE_WRITECOPY |
                PAGE_EXECUTE_READ |
                PAGE_EXECUTE_READWRITE |
                PAGE_EXECUTE_WRITECOPY
            );

            const auto is_guarded = mbi.Protect & (
                PAGE_GUARD |
                PAGE_NOACCESS
            );

            return is_rwe && !is_guarded;
        }
        return false;
    }
}
