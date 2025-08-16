#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace koalabox::zip {
    namespace fs = std::filesystem;

    /**
     * Extract contents of zip file
     * @param zip_path
     * @param predicate A function that should return path where the current file should be
     *                  extracted. Returning an empty path means the file won't be extracted.
     */
    void extract_files(
        const fs::path& zip_path,
        const std::function<fs::path(const std::string& name, bool is_dir)>& predicate
    );

}