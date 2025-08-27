#pragma once

namespace koalabox::patcher {
    uintptr_t find_pattern_address(
        uintptr_t base_address,
        size_t scan_size,
        const std::string& name,
        const std::string& pattern
    );
}