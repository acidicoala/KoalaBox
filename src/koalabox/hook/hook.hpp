#pragma once

#include "koalabox/win_util/win_util.hpp"
#include "koalabox/util/util.hpp"

namespace koalabox::hook {

    using FunctionPointer = char*;

    extern Map<String, uint64_t> address_book;

    [[maybe_unused]]
    void detour(HMODULE module, const char* function_name, FunctionPointer callback_function);

    [[maybe_unused]]
    void init(const std::function<void()>& callback);

    [[maybe_unused]]
    bool is_hook_mode(HMODULE self_module, const String& orig_library_name);

    template<typename RetType, typename... ArgTypes>
    [[maybe_unused]]
    auto get_original_function(
        bool is_hook_mode,
        HMODULE module,
        RetType(__stdcall*)(ArgTypes...),
        LPCSTR undecorated_function_name,
        bool mangle_function_names = false,
        int arg_bytes = 0
    ) {
        using func_type = RetType(__stdcall*)(ArgTypes...);

        if (is_hook_mode) {
            if (not address_book.contains(undecorated_function_name)) {
                util::panic(__func__,
                    "Address book does not contain function: {}", undecorated_function_name
                );
            }

            return (func_type) address_book[undecorated_function_name];
        } else {
            if (arg_bytes == 0) {
                // Guess byte count
                arg_bytes = 4 * sizeof...(ArgTypes);
            }

            std::string function_name = (not mangle_function_names || util::is_64_bit())
                ? undecorated_function_name
                : fmt::format("_{}@{}", undecorated_function_name, arg_bytes);

            return (func_type) win_util::get_proc_address(module, function_name.c_str());
        }
    }
}
