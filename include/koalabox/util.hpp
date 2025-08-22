#pragma once

// TODO: Remove this dependency
#include <spdlog/fmt/fmt.h>

namespace koalabox::util {

    constexpr auto BITNESS = 8 * sizeof(void*);

    void error_box(const std::string& title, const std::string& message);

    [[noreturn]] void panic(std::string message);

    template <typename... Args>
    [[noreturn]] void panic(fmt::format_string<Args...> fmt, Args&&... args) {
        const auto message = fmt::format(fmt, std::forward<Args>(args)...);

        panic(message);
    }

    // TODO: Remove this
    template <typename... Args>
    std::runtime_error exception(fmt::format_string<Args...> fmt, Args&&... args) {
        return std::runtime_error(fmt::format(fmt, std::forward<Args>(args)...));
    }

    bool is_valid_pointer(const void* pointer);

}
