#include "koalabox.hpp"

namespace koalabox::path {
    string to_platform_str(const fs::path& path) {
        return to_str(path);
    }
}
