#include <algorithm>
#include <koalabox/str.hpp>

namespace koalabox::str {
    std::string_view trim(const std::string_view& s) {
        const auto start = std::ranges::find_if_not(
            s, //
            [](const unsigned char ch) { return std::isspace(ch); }
        );

        const auto end = std::find_if_not(
                             s.rbegin(), //
                             s.rend(),   //
                             [](const unsigned char ch) { return std::isspace(ch); }
        ).base();

        return start < end ? std::string_view(start, end) : std::string_view();
    }

    std::string trim(const std::string& s) {
        return std::string(trim(std::string_view(s)));
    }

    std::string to_lower(const std::string& str) {
        std::string result;
        result.resize(str.size());

        std::ranges::transform(
            str, result.begin(), //
            [](const unsigned char c) { return std::tolower(c); }
        );

        return result;
    }

    bool eq(const std::string& s1, const std::string& s2) {
        return to_lower(s1) == to_lower(s2);
    }

    std::string to_str(const std::wstring& wstr) {
        if (wstr.empty()) {
            return {};
        }

        const auto required_size = WideCharToMultiByte(
            CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr
        );

        std::string result(required_size, 0);
        WideCharToMultiByte(
            CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), result.data(), required_size,
            nullptr, nullptr
        );

        return result;
    }

    std::wstring to_wstr(const std::string& str) {
        if (str.empty()) {
            return {};
        }

        const auto required_size =
            MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);

        std::wstring result(required_size, 0);
        MultiByteToWideChar(
            CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), required_size
        );

        return result;
    }
}