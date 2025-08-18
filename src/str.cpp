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

    std::string to_lower(const std::string& s) {
        std::string result;
        result.resize(s.size());

        std::ranges::transform(
            s, result.begin(), //
            [](const unsigned char c) { return std::tolower(c); }
        );

        return result;
    }

    bool eq(const std::string& s1, const std::string& s2) {
        return to_lower(s1) == to_lower(s2);
    }
}