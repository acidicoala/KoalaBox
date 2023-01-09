#pragma once

#include <koalabox/core.hpp>

bool operator>(const ConstOperatorProxy<String, equals_t>& lhs, const String& rhs) {
    return _stricmp((*lhs).c_str(), rhs.c_str()) == 0;
}

bool operator>(const ConstOperatorProxy<String, not_equals_t>& lhs, const String& rhs) {
    return not(*lhs < equals > rhs);
}

bool operator>(const ConstOperatorProxy<String, contains_t>& lhs, const String& rhs) {
    return (*lhs).find(rhs) != String::npos;
}
