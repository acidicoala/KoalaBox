#pragma once

#include "../koalabox.hpp"

#include <fmt/core.h>
#include <Windows.h>

namespace koalabox::util {

    [[maybe_unused]]
    void error_box(String title, String message);

    [[maybe_unused]]
    Path get_module_dir(HMODULE& handle);

    [[maybe_unused]]
    bool is_64_bit();

    [[maybe_unused]]
    void panic(String title, String message);

    template<typename... Args>
    [[maybe_unused]]
    void panic(String title, fmt::format_string<Args...> fmt, Args&& ...args) {
        title = fmt::format("[{}] {}", project_name, title);
        auto message = fmt::format(fmt, std::forward<Args>(args)...);

        panic(title, message);
    }

    [[maybe_unused]]
    bool strings_are_equal(const String& string1, const String& string2);

    [[maybe_unused]]
    String to_string(const WideString& wstr);

    [[maybe_unused]]
    WideString to_wstring(const String& str);

}
