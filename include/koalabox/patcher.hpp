#pragma once

namespace koalabox::patcher {
    uintptr_t find_pattern_address(
        uintptr_t base_address,
        size_t scan_size,
        const std::string& name,
        const std::string& pattern
    );

    uintptr_t find_pattern_address(
        const MODULEINFO& process_info,
        const std::string& name,
        const std::string& pattern
    );

    uintptr_t find_pattern_address(
        HMODULE module_handle,
        const std::string& section_name,
        const std::string& name,
        const std::string& pattern
    );
}