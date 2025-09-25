#include <fstream>
#include <iostream>
#include <regex>
#include <set>

#include <cxxopts.hpp>
#include <glob/glob.h>

#include <koalabox/io.hpp>
#include <koalabox/lib.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/parser.hpp>
#include <koalabox/path.hpp>

#include <koalabox_tools/cmd.hpp>

namespace {
    namespace kb = koalabox;
    namespace fs = std::filesystem;

    struct Args {
        bool demangle;
        std::string forwarded_dll_name;
        std::string lib_files_glob;
        std::string output_file_path;
        std::string sources_input_path;

        static Args parse(const int argc, const TCHAR** argv) {
            const auto normalized = kb::tools::cmd::normalize_args(argc, argv);

            KBT_CMD_PARSE_ARGS(
                "linux_exports_generator", "Generates proxy exports for linux libraries",
                argc, normalized.argv.data(),
                demangle,
                forwarded_dll_name,
                lib_files_glob,
                output_file_path,
                sources_input_path
            )

            return args;
        }
    };

    std::string preprocess_source_file(const std::string& source_file_content) {
        const std::regex dll_export_regex(R"(DLL_EXPORT\(\s*([^)]+?)\s*\))");
        return std::regex_replace(source_file_content, dll_export_regex, "$1");
    }

    /**
     * Returns a list of functions parsed from the sources
     * in a given directory. Edge cases: Comments
     */
    auto get_defined_functions(const fs::path& path) {
        std::set<std::string> declared_functions;

        for(const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            const auto& file_path = entry.path();
            if(file_path.extension() != ".cpp") {
                continue;
            }

            LOG_DEBUG("Processing source file: {}", kb::path::to_str(file_path));

            const auto file_content = koalabox::io::read_file(file_path);

            const auto processed_source = preprocess_source_file(file_content);

            const auto query_results = koalabox::parser::query(
                processed_source,
                // language=regexp
                std::regex(R"((/\w+)*/function_declarator/identifier)")
            );

            for(const auto& [_, value] : query_results) {
                LOG_DEBUG("Found function: {}", value);
                declared_functions.insert(std::string(value));
            }
        }

        return declared_functions;
    }

    auto get_library_exports_map(const std::string& lib_files_glob, const bool undecorate) {
        kb::lib::export_map_t dll_exports;

        const auto lib_path_list = glob::glob(lib_files_glob);
        LOG_INFO("Found {} library files", lib_path_list.size());

        for(const auto& lib_path : lib_path_list) {
            if(not fs::exists(lib_path)) {
                continue;
            }

            const auto* const library = kb::lib::load_library_or_throw(lib_path);
            const auto lib_exports = kb::lib::get_export_map(library, undecorate);

            dll_exports.insert(lib_exports.begin(), lib_exports.end());
        }

        LOG_INFO("Found {} exported functions", dll_exports.size());

        return dll_exports;
    }

    void generate_proxy_exports(
        std::ofstream& export_file,
        const std::map<std::string, std::string>& lib_exports,
        const std::set<std::string>& defined_functions,
        const std::string& forwarded_dll_name
    ) {
        // Add header guard
        export_file << "#pragma once" << std::endl << std::endl;

        for(const auto& [function_name, decorated_function_name] : lib_exports) {
            // Comment out exports that we have defined
            const std::string comment = defined_functions.contains(function_name) ? "//" : "";

            const auto line = std::format(
                R"({}#pragma comment(linker, "/export:{}={}.{}"))",
                //
                comment,
                decorated_function_name,
                forwarded_dll_name,
                decorated_function_name
            );

            export_file << line << std::endl;
        }
    }
}

/**
 * Args <br>
 * 0: program name <br>
 * 1: undecorate? <br>
 * 2: forwarded dll name <br>
 * 3: glob pattern for input dll paths
 * 4: output file path <br>
 * 5: sources input path <br> (optional)
 */
int MAIN(const int argc, const TCHAR* argv[]) { // NOLINT(*-use-internal-linkage)
    try {
        koalabox::logger::init_console_logger();

        const auto args = Args::parse(argc, argv);

        const auto output_file_path = kb::path::from_str(args.output_file_path);

        // Input sources are optional because SmokeAPI and Koaloader don't have them.
        const auto sources_input_path = kb::path::from_str(args.sources_input_path);

        const auto defined_functions = sources_input_path.empty()
                                           ? std::set<std::string>()
                                           : get_defined_functions(sources_input_path);

        // Create directories for export file, if necessary
        fs::create_directories(output_file_path.parent_path());

        // Open the export header file for writing
        std::ofstream export_file(output_file_path, std::ofstream::out | std::ofstream::trunc);
        if(not export_file.is_open()) {
            LOG_ERROR("Filed to open header file for writing");
            exit(EXIT_FAILURE);
        }

        const auto lib_exports = get_library_exports_map(args.lib_files_glob, args.demangle);
        generate_proxy_exports(export_file, lib_exports, defined_functions, args.forwarded_dll_name);

        LOG_INFO("Finished generating {}", kb::path::to_str(output_file_path));
    } catch(const std::exception& ex) {
        LOG_ERROR("Error: {}", ex.what());
        exit(EXIT_FAILURE);
    }

    koalabox::logger::shutdown();
    return EXIT_SUCCESS;
}
