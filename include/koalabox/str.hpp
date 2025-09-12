#pragma once

#include <string>

#include "koalabox/koalabox.hpp"

namespace koalabox::str {
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
    std::string to_str(const string& str);

    /**
     * Converts 1-byte string to 2-byte wide string.
     */
    std::wstring to_wstr(const std::string& str);

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

    std::wstring to_wstr(const std::u16string& u16str);

    std::u16string to_u16str(const std::wstring& wstr);
}
