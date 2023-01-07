#pragma once

#include <koalabox/core.hpp>

#include <spdlog/fmt/fmt.h>

namespace koalabox::util {

    KOALABOX_API(void) error_box(const String& title, const String& message);

    [[noreturn]] KOALABOX_API(void) panic(String message);

    template<typename... Args>
    [[noreturn]] KOALABOX_API(void) panic(fmt::format_string<Args...> fmt, Args&& ... args) {
        const auto message = fmt::format(fmt, std::forward<Args>(args)...);

        panic(message);
    }

    KOALABOX_API(bool) strings_are_equal(const String& string1, const String& string2);

    KOALABOX_API(String) to_string(const WideString& wstr);

    KOALABOX_API(WideString) to_wstring(const String& str);

    template<typename... Args>
    KOALABOX_API(Exception) exception(fmt::format_string<Args...> fmt, Args&& ...args) {
        const auto message = fmt::format(fmt, std::forward<Args>(args)...);

        return Exception(message.c_str());
    }

    KOALABOX_API(bool) is_valid_pointer(const void* pointer);

}
