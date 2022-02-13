#pragma once

#include "koalabox/koalabox.hpp"
#include "koalabox/win_util/win_util.hpp"

#include <build_config.h>

namespace koalabox::util {

    [[maybe_unused]]
    void error_box(String title, String message);

    [[maybe_unused]]
    bool is_64_bit();

    [[maybe_unused]]
    void panic(String title, String message);

    template<typename... Args>
    [[maybe_unused]]
    void panic(String title, fmt::format_string<Args...> fmt, Args&& ...args) {
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
