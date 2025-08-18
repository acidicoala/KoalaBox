#pragma once

#include <string>

namespace koalabox::str {
    std::string_view trim(const std::string_view& s);
    std::string trim(const std::string& s);

    /**
     * Performs case-insensitive string comparison.
     * For case-sensitive comparison regular == operator should be used.
     */
    bool eq(const std::string& s1, const std::string& s2);

    /**
     * Returns a lowercase copy of the given string.
     */
    std::string to_lower(const std::string& str);

    /**
     * Converts 2-byte wide string to 1-byte string.
     */
    std::string to_str(const std::wstring& wstr);

    /**
     * Converts 1-byte string to 2-byte wide string.
     */
    std::wstring to_wstr(const std::string& str);
}