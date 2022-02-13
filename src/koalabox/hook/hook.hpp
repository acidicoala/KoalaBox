#pragma once

#include "koalabox/win_util/win_util.hpp"
#include "koalabox/util/util.hpp"

namespace koalabox::hook {

    using FunctionPointer = char*;

    extern Map<String, FunctionPointer> address_book;

    [[maybe_unused]]
    void detour(
        HMODULE module,
        const String& function_name,
        FunctionPointer callback_function,
        bool panic_on_fail = false
    );

    [[maybe_unused]]
    void init(const std::function<void()>& callback);

    [[maybe_unused]]
    bool is_hook_mode(HMODULE self_module, const String& orig_library_name);

}
