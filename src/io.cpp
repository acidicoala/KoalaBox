#include <fstream>

#include "koalabox/io.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/path.hpp"

namespace koalabox::io {
    std::string read_file(const fs::path& file_path) {
        std::ifstream input_stream(file_path);
        input_stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        return {std::istreambuf_iterator{input_stream}, {}};
    }

    bool write_file(const fs::path& file_path, const std::string& contents) noexcept {
        try {
            if(!std::filesystem::exists(file_path)) {
                std::filesystem::create_directories(file_path.parent_path());
            }

            // Use binary mode to prevent windows from writing \n as CRLF
            std::ofstream output_stream(file_path, std::ios::binary);
            if(output_stream.good()) {
                output_stream << contents;

                LOG_TRACE(R"(Writing file to disk: "{}")", path::to_str(file_path));
                return true;
            }

            LOG_ERROR(
                R"(Error opening output stream: "{}". Flags: {})",
                path::to_str(file_path),
                static_cast<uint32_t>(output_stream.flags())
            );
            return false;
        } catch(const std::exception& e) {
            LOG_ERROR("Unexpected exception caught: {}", e.what());
            return false;
        }
    }
}