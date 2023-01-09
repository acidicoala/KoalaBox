#include <koalabox/io.hpp>
#include <koalabox/logger.hpp>

#include <fstream>

namespace koalabox::io {

    String read_file(const Path& file_path) {
        std::ifstream input_stream(file_path);
        input_stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try {
            return {std::istreambuf_iterator<char>{input_stream}, {}};
        } catch (const std::system_error& e) {
            const auto& code = e.code();
            throw std::runtime_error(
                fmt::format("Input file stream error code: {}, message: {}", code.value(), code.message())
            );
        }
    }

    bool write_file(const Path& file_path, const String& contents) noexcept {
        try {

            if (!std::filesystem::exists(file_path)) {
                std::filesystem::create_directories(file_path.parent_path());
            }

            std::ofstream output_stream(file_path);
            if (output_stream.good()) {
                output_stream << contents;

                LOG_DEBUG(R"(Writing file to disk: "{}")", file_path.string())
                return true;
            }

            LOG_ERROR(R"(Error saving file: "{}")", file_path.string())
            return false;
        } catch (const Exception& e) {
            LOG_ERROR("Unexpected exception caught: {}", e.what())
            return false;
        }
    }

}
