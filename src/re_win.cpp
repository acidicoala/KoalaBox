#include "koalabox/re.hpp"

// RtlLookupFunctionEntry / RUNTIME_FUNCTION come from <Windows.h>, force-included via the PCH.

namespace koalabox::re {
    std::optional<uintptr_t> get_function_start(const uintptr_t address) {
        DWORD64 image_base = 0;
        auto* const function_entry = RtlLookupFunctionEntry(address, &image_base, nullptr);
        if(function_entry == nullptr) {
            return std::nullopt;
        }

        return static_cast<uintptr_t>(image_base) + function_entry->BeginAddress;
    }
}
