#pragma once

#include "koalabox/util/util.hpp"
#include "koalabox/win_util/win_util.hpp"

namespace koalabox::hook {

    extern Map<String, FunctionPointer> address_book;

    void detour_or_throw(
        const HMODULE& module,
        const String& function_name,
        FunctionPointer callback_function,
        PLH::x64Detour::detour_scheme_t scheme = PLH::x64Detour::RECOMMENDED
    );

    void detour_or_warn(
        const HMODULE& module,
        const String& function_name,
        FunctionPointer callback_function,
        PLH::x64Detour::detour_scheme_t scheme = PLH::x64Detour::RECOMMENDED
    );

    void detour(
        const HMODULE& module,
        const String& function_name,
        FunctionPointer callback_function,
        PLH::x64Detour::detour_scheme_t scheme = PLH::x64Detour::RECOMMENDED
    );

    void eat_hook_or_throw(
        const HMODULE& module,
        const String& function_name,
        FunctionPointer callback_function
    );

    void eat_hook_or_warn(
        const HMODULE& module,
        const String& function_name,
        FunctionPointer callback_function
    );

    void swap_virtual_func_or_throw(
        const void* instance,
        const String& function_name,
        int ordinal,
        FunctionPointer callback_function
    );

    void swap_virtual_func(
        const void* instance,
        const String& function_name,
        int ordinal,
        FunctionPointer callback_function
    );

    FunctionPointer get_original_function(bool is_hook_mode, const HMODULE& library, const String& function_name);

    template<typename F>
    F get_original_function(bool is_hook_mode, const HMODULE& library, const String& function_name, F) {
        return reinterpret_cast<F>(get_original_function(is_hook_mode, library, function_name));
    }

    void init(bool print_info = false);

    bool is_hook_mode(const HMODULE& self_module, const String& orig_library_name);

}
