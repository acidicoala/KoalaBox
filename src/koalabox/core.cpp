#pragma once

#include <koalabox/core.hpp>

// Be warned, lurker. Dark sorcery await ahead of you...

bool operator>(const OperatorProxy<String, equals_t>& lhs, const String& rhs) {
    return _stricmp((*lhs).c_str(), rhs.c_str()) == 0;
}

bool operator>(const OperatorProxy<String, not_equals_t>& lhs, const String& rhs) {
    return not(*lhs < equals > rhs);
}

bool operator>(const OperatorProxy<String, contains_t>& lhs, const String& rhs) {
    return (*lhs).find(rhs) != String::npos;
}
