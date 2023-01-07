#pragma once

#include <koalabox/core.hpp>

namespace koalabox::patcher {

    KOALABOX_API(uintptr_t) find_pattern_address(
        uintptr_t base_address,
        size_t scan_size,
        const String& name,
        const String& pattern
    );

    KOALABOX_API(uintptr_t) find_pattern_address(
        const MODULEINFO& process_info,
        const String& name,
        const String& pattern
    );

}
