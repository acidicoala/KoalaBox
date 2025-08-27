#pragma once

#include <string>

namespace koalabox::util {
    constexpr auto BITNESS = 8 * sizeof(void*);

    void error_box(const std::string& title, const std::string& message);

    [[noreturn]] void panic(const std::string& message);
}