#pragma once

#include <koalabox/koalabox.hpp>

namespace koalabox::patcher {

    void* find_pattern_address(
        const uint8_t* base_address,
        size_t scan_size,
        const String& name,
        const String& pattern,
        bool log_results
    );
    void* find_pattern_address(const MODULEINFO& process_info, const String& name, const String& pattern);

}
