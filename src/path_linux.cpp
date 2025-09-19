#include <filesystem>

#include "koalabox/path.hpp"
#include "koalabox/str.hpp"

namespace koalabox::path {
    str::platform_string to_platform_str(const std::filesystem::path& path) {
        return to_str(path);
    }
}
