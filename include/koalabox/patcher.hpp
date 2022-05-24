#pragma once

#include <koalabox/koalabox.hpp>

namespace koalabox::patcher {

    void* find_pattern_address(const MODULEINFO& process_info, const String& name, const String& pattern);

}
