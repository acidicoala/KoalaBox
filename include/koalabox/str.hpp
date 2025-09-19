#pragma once

#include <string>

#include "koalabox/core.hpp"

namespace koalabox::str {
#ifdef KB_WIN
    using platform_string = std::wstring;
#elifdef KB_LINUX
    using platform_string = std::string;
#endif

    struct case_insensitive_compare {
        bool operator()(const std::string& str1, const std::string& str2) const noexcept;
    };

    /**
     * @return A new string with leading and trailing spaces removed
     */
    std::string trim(std::string s);

    /**
     * Performs case-insensitive string comparison.
     * For case-sensitive comparison the regular <code> ==</code> operator should be used.
     */
    bool eq(const std::string& s1, const std::string& s2);

    /**
     * Returns a lowercase copy of the given string.
     */
    std::string to_lower(const std::string& str);

    /**
     * Converts platform string to 1-byte string.
     */
    std::string to_str(const platform_string& str);

    /**
     * Converts an ASCII string to hexadecimal representation.
     *
     * Example input: <code>"Hello World"</code><br>
     * Example output: <code>"48 65 6C 6C 6F 20 57 6F 72 6C 64"</code>
     */
    std::string to_hex(const std::string& str);

    /**
     * Converts a 4-byte value to a hex string with little-endian encoding.
     *
     * Example input: <code>0x7A9306D4</code><br>
     * Target output: <code>"D4 06 93 7A"</code>
     */
    std::string from_little_endian(uint32_t number);

#ifdef KB_WIN
    /** Converts 1-byte string to 2-byte wide string. */
    std::wstring to_wstr(const std::string& str);
    std::wstring to_wstr(const std::u16string& u16str);
    std::u16string to_u16str(const std::wstring& wstr);
#endif
}
