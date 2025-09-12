#include <wil/stl.h>
#include <wil/win32_helpers.h>

#include "koalabox/module.hpp"
#include "koalabox/path.hpp"

namespace koalabox::module {
    std::filesystem::path get_fs_path(const void* const module_handle) {
        const auto wstr_path = wil::GetModuleFileNameW<std::wstring>(module_handle);
        return path::from_wstr(wstr_path);
    }

    std::optional<void*> get_function_address(
        const void* const module_handle,
        const char* function_name,
        const char* /*version*/
    ) {
        if(auto* const address = GetProcAddress(module_handle, function_name)) {
            return {address};
        }

        return {};
    }
}
