#include "koalabox/path.hpp"

namespace koalabox::path {
    string to_kb_str(const fs::path& path) {
        return to_str(path);
    }
}
