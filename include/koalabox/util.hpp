#pragma once

#include <koalabox/types.hpp>
#include <spdlog/fmt/fmt.h>

namespace koalabox::util {
    void error_box(const String& title, const String& message);

    bool is_x64();

    [[noreturn]] void panic(String message);

    template<typename... Args>
    [[noreturn]] void panic(fmt::format_string<Args...> fmt, Args&& ... args) {
        const auto message = fmt::format(fmt, std::forward<Args>(args)...);

        panic(message);
    }

    bool strings_are_equal(const String& string1, const String& string2);

    String to_string(const WideString& wstr);

    WideString to_wstring(const String& str);

    template<typename... Args>
    Exception exception(fmt::format_string<Args...> fmt, Args&& ...args) {
        const auto message = fmt::format(fmt, std::forward<Args>(args)...);

        return Exception(message.c_str());
    }

    bool is_valid_pointer(const void* pointer);

}
