#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <string>

#include <miniz.h>

#include "koalabox/path.hpp"
#include "koalabox/zip.hpp"

namespace koalabox::zip {
    void extract_files(
        const fs::path& zip_path,
        const std::function<fs::path(const std::string& name, bool is_dir)>& predicate
    ) {
        mz_zip_archive zip{};

        const auto zip_path_str = path::to_str(zip_path);
        if(!mz_zip_reader_init_file(&zip, zip_path_str.c_str(), 0)) {
            throw std::runtime_error(
                "mz_zip_reader_init_file() failed for: " + path::to_str(zip_path)
            );
        }

        [[maybe_unused]] auto guard = [&zip] {
            mz_zip_reader_end(&zip);
        };
        try {
            std::error_code ec;
            const mz_uint num_files = mz_zip_reader_get_num_files(&zip);

            for(mz_uint i = 0; i < num_files; ++i) {
                mz_zip_archive_file_stat st;
                if(!mz_zip_reader_file_stat(&zip, i, &st)) {
                    mz_zip_reader_end(&zip);
                    throw std::runtime_error(
                        "mz_zip_reader_file_stat() failed at index " + std::to_string(i)
                    );
                }

                const std::string name = st.m_filename;

                // Skip dangerous names early
                if(name.empty()) {
                    continue;
                }

                const bool is_dir = mz_zip_reader_is_file_a_directory(&zip, i) != 0;

                const auto out_path = predicate(name, is_dir);
                if(out_path.empty()) {
                    continue;
                }

                const auto out_path_str = path::to_str(out_path);

                if(is_dir) {
                    fs::create_directories(out_path, ec);
                    if(ec) {
                        mz_zip_reader_end(&zip);
                        throw std::runtime_error(
                            std::format(
                                "Failed to create directory: {}. Reason: {}",
                                out_path_str,
                                ec.message()
                            )
                        );
                    }
                    continue;
                }

                // Ensure parent directories exist
                fs::create_directories(out_path.parent_path(), ec);
                if(ec) {
                    mz_zip_reader_end(&zip);
                    throw std::runtime_error(
                        std::format(
                            "Failed to create parent directories for: {}. Reason: {}",
                            out_path_str,
                            ec.message()
                        )
                    );
                }

                // Extract to heap then write to file
                size_t uncompressed_size = 0;
                void* p = mz_zip_reader_extract_to_heap(&zip, i, &uncompressed_size, 0);
                if(!p) {
                    mz_zip_reader_end(&zip);
                    throw std::runtime_error("Extraction failed for entry: " + name);
                }

                std::ofstream output_stream(out_path, std::ios::binary);
                if(!output_stream) {
                    mz_free(p);
                    mz_zip_reader_end(&zip);
                    throw std::runtime_error(
                        "Failed to open output file for writing: " + out_path_str
                    );
                }
                output_stream.write(
                    static_cast<const char*>(p),
                    uncompressed_size
                );
                output_stream.close();
                mz_free(p);

                if(!output_stream) {
                    mz_zip_reader_end(&zip);
                    throw std::runtime_error("Failed to write output file: " + out_path_str);
                }
            }

            mz_zip_reader_end(&zip);
        } catch(...) {
            // Ensure cleanup on exceptions
            guard();
            throw;
        }
    }
}
