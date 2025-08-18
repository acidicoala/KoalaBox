#include <deque>
#include <fstream>
#include <iostream>
#include <regex>

#include <glob/glob.h>

#include <koalabox/io.hpp>
#include <koalabox/loader.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/parser.hpp>
#include <koalabox/str.hpp>
#include <koalabox/util.hpp>
#include <koalabox/win_util.hpp>

namespace kb = koalabox;

namespace {
    namespace fs = std::filesystem;

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

        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            const auto& file_path = entry.path();
            if (file_path.extension() != ".cpp") {
                continue;
            }

            LOG_DEBUG("Processing source file: {}", file_path.string());

            const auto file_content = koalabox::io::read_file(file_path);

            const auto processed_source = preprocess_source_file(file_content);

            const auto query_results = koalabox::parser::query(
                processed_source,
                // language=regexp
                std::regex(R"((/\w+)*/function_declarator/identifier)")
            );

            for (const auto& [_, value] : query_results) {
                LOG_DEBUG("Found function: {}", value);
                declared_functions.insert(std::string(value));
            }
        }

        return declared_functions;
    }

    bool parseBoolean(const String& bool_str) {
        if (bool_str == "TRUE") {
            return true;
        }

        if (bool_str == "FALSE") {
            return false;
        }

        LOG_ERROR("Invalid boolean value: {}", bool_str);
        exit(10);
    }

    auto get_dll_exports(const std::string& dll_files_glob, const bool undecorate) {
        std::map<std::string, std::string> dll_exports;

        const auto dll_path_list = glob::glob(dll_files_glob);
        LOG_INFO("Found {} DLL files", dll_path_list.size());

        for (const auto& dll_path : dll_path_list) {
            if (not fs::exists(dll_path)) {
                continue;
            }

            auto* const library = kb::win_util::load_library_or_throw(dll_path);
            const auto lib_exports = kb::loader::get_export_map(library, undecorate);

            dll_exports.insert(lib_exports.begin(), lib_exports.end());
        }

        LOG_INFO("Found {} exported functions", dll_exports.size());

        return dll_exports;
    }
}

/**
 * Args <br>
 * 0: program name <br>
 * 1: undecorate? <br>
 * 2: forwarded dll name <br>
 * 3: glob pattern for input dll paths
 * 4: header output path <br>
 * 5: sources input path <br> (optional)
 */
int wmain(const int argc, const wchar_t* argv[]) { // NOLINT(*-use-internal-linkage)
    try {
        koalabox::logger::init_console_logger();

        for (int i = 0; i < argc; i++) {
            LOG_INFO("Arg #{} = '{}'", i, koalabox::str::to_str(argv[i]));
        }

        if (argc != 5 && argc != 6) {
            LOG_ERROR("Invalid number of arguments. Expected 5 or 6. Got: {}", argc);

            exit(1);
        }

        const auto undecorate = parseBoolean(kb::str::to_str(argv[1]));
        const auto forwarded_dll_name = kb::str::to_str(argv[2]);
        const auto input_dll_glob = kb::str::to_str(argv[3]);
        const auto header_output_path = fs::path(argv[4]);

        // Input sources are optional because Koaloader doesn't have them.
        const auto sources_input_path = fs::path(argc == 6 ? argv[5] : L"");

        const auto defined_functions = sources_input_path.empty()
                                           ? std::set<std::string>()
                                           : get_defined_functions(sources_input_path);

        const auto dll_exports = get_dll_exports(input_dll_glob, undecorate);

        // Create directories for export header, if necessary
        fs::create_directories(header_output_path.parent_path());

        // Open the export header file for writing
        std::ofstream export_file(header_output_path, std::ofstream::out | std::ofstream::trunc);
        if (not export_file.is_open()) {
            LOG_ERROR("Filed to open header file for writing");
            exit(4);
        }

        // Add header guard
        export_file << "#pragma once" << std::endl << std::endl;

        for (const auto& [function_name, decorated_function_name] : dll_exports) {
            // Comment out exports that we have defined
            const std::string comment = defined_functions.contains(function_name) ? "//" : "";

            const auto line = std::format(
                R"({}#pragma comment(linker, "/export:{}={}.{}"))", //
                comment, decorated_function_name, forwarded_dll_name, decorated_function_name
            );

            export_file << line << std::endl;
        }
    } catch (const Exception& ex) {
        LOG_ERROR("Error: {}", ex.what());
        exit(-1);
    }

    koalabox::logger::shutdown();
}