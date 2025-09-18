#pragma once

#include <koalabox/core.hpp>

namespace koalabox::util {
    constexpr auto BITNESS = 8 * sizeof(void*);

    void error_box(const std::string& title, const std::string& message);

    [[noreturn]] void panic(const std::string& message);

    std::string get_env_var(const std::string& key);

#ifdef KB_WIN
    bool is_wine_env();
#endif
}
