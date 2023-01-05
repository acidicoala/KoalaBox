#include <koalabox/io.hpp>

#include <fstream>

namespace koalabox::io {

    String read_file(const Path& file_path) {
        std::ifstream input_stream(file_path);

        return input_stream.good()
               ? String(std::istreambuf_iterator<char>{input_stream}, {})
               : "";
    }

    bool write_file(const Path& file_path, const String& contents) {
        if (!std::filesystem::exists(file_path)) {
            std::filesystem::create_directories(file_path.parent_path());
        }

        std::ofstream output_stream(file_path);
        if (output_stream.good()) {
            output_stream << contents;

            logger->debug("{} -> Saved file to disk: '{}'", __func__, file_path.string());

            return true;
        }

        logger->debug("{} -> Error saving file: '{}'", __func__, file_path.string());
        return false;
    }

}
