#pragma once

#include <koalabox/types.hpp>

namespace koalabox::io {

    String read_file(const Path& file_path);

    bool write_file(const Path& file_path, const String& contents);

}
