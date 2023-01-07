#pragma once

#include <koalabox/types.hpp>

namespace koalabox::patcher {

    uintptr_t find_pattern_address(
        uintptr_t base_address,
        size_t scan_size,
        const String& name,
        const String& pattern
    );

    uintptr_t find_pattern_address(
        const MODULEINFO& process_info,
        const String& name,
        const String& pattern
    );
}
