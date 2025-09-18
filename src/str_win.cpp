#include "koalabox/str.hpp"

namespace koalabox::str {
    // TODO: try utf8::utf16to8(u16str);
    std::string to_str(const std::wstring& str) {
        if(str.empty()) {
            return {};
        }

        const auto required_size = WideCharToMultiByte(
            CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0, nullptr, nullptr
        );

        std::string result(required_size, 0);
        WideCharToMultiByte(
            CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), required_size, nullptr, nullptr
        );

        return result;
    }

    // TODO: try utf8::utf8to16(u8str);
    std::wstring to_wstr(const std::string& str) {
        if(str.empty()) {
            return {};
        }
        const auto required_size = MultiByteToWideChar(
            CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0
        );

        std::wstring result(required_size, 0);
        MultiByteToWideChar(
            CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), required_size
        );

        return result;
    }

    std::wstring to_wstr(const std::u16string& u16str) {
        // See warning in to_u16str
        return {u16str.begin(), u16str.end()};
    }

    std::u16string to_u16str(const std::wstring& wstr) {
        // This works only for windows. See https://stackoverflow.com/a/42734882/31250678
        // But then again, we're in a namespace for windows utils anyway ¯\_(ツ)_/¯
        return {wstr.begin(), wstr.end()};
    }
}
