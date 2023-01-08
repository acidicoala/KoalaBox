#pragma once

#include <koalabox/core.hpp>

namespace koalabox::io {

    String read_file(const Path& file_path);

    /**
     * Write a string to file at the given path.
     * @return `true` if operation was successful, `false` otherwise
     */
    bool write_file(const Path& file_path, const String& contents) noexcept;

}
