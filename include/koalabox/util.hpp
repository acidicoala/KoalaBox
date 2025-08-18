#pragma once

#include <koalabox/core.hpp>

#include <spdlog/fmt/fmt.h>

#define KException koalabox::util::exception

namespace koalabox::util {

    KOALABOX_API(void) error_box(const String& title, const String& message);

    [[noreturn]] KOALABOX_API(void) panic(String message);

    template<typename... Args>
    [[noreturn]] KOALABOX_API(void) panic(fmt::format_string<Args...> fmt, Args&& ... args) {
        const auto message = fmt::format(fmt, std::forward<Args>(args)...);

        panic(message);
    }

    template<typename... Args>
    KOALABOX_API(Exception) exception(fmt::format_string<Args...> fmt, Args&& ...args) {
        return std::runtime_error(fmt::format(fmt, std::forward<Args>(args)...));
    }

    KOALABOX_API(bool) is_valid_pointer(const void* pointer);

}
