#pragma once

#include <koalabox/core.hpp>

namespace koalabox::io {

    std::optional<String> read_file(const Path& file_path);

    bool write_file(const Path& file_path, const String& contents);

}
