#pragma once

#include "../koalabox.hpp"
#include "../win_util/win_util.hpp"
#include <build_config.h>

namespace koalabox::util {

    [[maybe_unused]]
    void error_box(String title, String message);

    template<typename F>
    [[maybe_unused]]
    F fn_cast(FARPROC func_to_cast, [[maybe_unused]] F func_cast_to) {
        return (F) func_to_cast;
    }

    template<typename F>
    [[maybe_unused]]
    F get_procedure(HMODULE module, LPCSTR procedure_name, F func_cast_to) {
        const auto address = win_util::get_proc_address(module, procedure_name);
        return fn_cast(address, func_cast_to);
    }

    [[maybe_unused]]
    bool is_64_bit();

    [[maybe_unused]]
    void panic(String title, String message);

    template<typename... Args>
    [[maybe_unused]]
    void panic(String title, fmt::format_string<Args...> fmt, Args&& ...args) {
        title = fmt::format("[{}] {}", PROJECT_NAME, title);
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
