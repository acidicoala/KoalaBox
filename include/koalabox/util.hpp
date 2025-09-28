#pragma once

#include <optional>
#include <string>

namespace koalabox::util {
    void error_box(const std::string& title, const std::string& message);

    [[noreturn]] void panic(const std::string& message);

    std::optional<std::string> get_env(const std::string& key) noexcept;

#ifdef KB_WIN
    bool is_wine_env();
#endif
}
