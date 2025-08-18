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
}