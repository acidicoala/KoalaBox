#pragma once

#include <filesystem>
#include <string>

namespace koalabox::io {
    namespace fs = std::filesystem;

    /**
     * @throws exception on file read error
     */
    std::string read_file(const fs::path& file_path);

    /**
     * Write a string to file at the given path.
     * @return `true` if operation was successful, `false` otherwise
     */
    bool write_file(const fs::path& file_path, const std::string& contents) noexcept;
}