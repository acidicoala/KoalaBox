#include "koalabox/koalabox.hpp"
#include "koalabox/module.hpp"

namespace koalabox::module {
    void* load_library_or_throw(const std::filesystem::path& library_path) {
        return load_library(library_path) | throw_if_empty("Failed to load library");
    }
}
