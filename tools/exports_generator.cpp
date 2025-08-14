#include <koalabox/util.hpp>
#include <koalabox/win_util.hpp>
#include <koalabox/loader.hpp>
#include <koalabox/logger.hpp>

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/pattern_formatter.h>
#include <glob/glob.h>

#include <fstream>
#include <regex>

using namespace koalabox;

namespace {
    namespace fs = std::filesystem;

    /**
     * Returns a list of functions parsed from the sources
     * in a given directory. Edge cases: Comments
     */
    Set<String> get_implemented_functions(const Path& path) {
        Set<String> implemented_functions;

        for (const auto& p : std::filesystem::recursive_directory_iterator(path)) {
            const auto& file_path = p.path();

            std::ifstream ifs(file_path);
            std::string file_content(std::istreambuf_iterator{ifs}, {});

            // TODO: Use tree-sitter instead of regex
            // Matches function name in the 1st group
            static const std::regex func_name_pattern(
                R"(\s*DLL_EXPORT\([\w|\s]+\**\)\s*(\w+)\s*\()");
            std::smatch match;
            while (regex_search(file_content, match, func_name_pattern)) {
                const auto func_name = match.str(1);

                if (not implemented_functions.contains(func_name)) {
                    implemented_functions.insert(func_name);
                    LOG_INFO("Implemented function: \"{}\"", func_name)
                }

                file_content = match.suffix();
            }
        }

        return implemented_functions;
    }

    bool parseBoolean(const String& bool_str) {
        if (bool_str == "TRUE") {
            return true;
        }

        if (bool_str == "FALSE") {
            return false;
        }

        LOG_ERROR("Invalid boolean value: {}", bool_str)
        exit(10);
    }

    auto get_dll_exports(const std::string& dll_files_glob, const bool undecorate) {
        std::map<std::string, std::string> dll_exports;

        const auto dll_path_list = glob::glob(dll_files_glob);
        LOG_INFO("Found {} DLL files", dll_path_list.size());

        for (auto& dll_path : dll_path_list) {
            if (not fs::exists(dll_path)) {
                continue;
            }

            auto* const library = win_util::load_library_or_throw(dll_path);
            const auto lib_exports = loader::get_export_map(library, undecorate);

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
        logger::instance = spdlog::stdout_logger_st("stdout");

        auto formatter = std::make_unique<spdlog::pattern_formatter>();
        formatter->set_pattern("%H:%M:%S.%e │ %l │ %v");
        logger::instance->set_formatter(std::move(formatter));
        logger::instance->flush_on(spdlog::level::trace);
        logger::instance->set_level(spdlog::level::trace);

        for (int i = 0; i < argc; i++) {
            LOG_INFO("Arg #{} = '{}'", i, util::to_string(argv[i]))
        }

        if (argc != 5 && argc != 6) {
            LOG_ERROR("Invalid number of arguments. Expected 5 or 6. Got: {}", argc)

            exit(1);
        }

        const auto undecorate = parseBoolean(util::to_string(argv[1]));
        const auto forwarded_dll_name = util::to_string(argv[2]);
        const auto input_dll_glob = util::to_string(argv[3]);
        const auto header_output_path = Path(argv[4]);

        // Input sources are optional because Koaloader doesn't have them.
        const auto sources_input_path = Path(argc == 6 ? argv[5] : L"");

        const auto implemented_functions =
            sources_input_path.empty()
                ? Set<String>()
                : get_implemented_functions(sources_input_path);

        const auto dll_exports = get_dll_exports(input_dll_glob, undecorate);

        // Create directories for export header, if necessary
        fs::create_directories(header_output_path.parent_path());

        // Open the export header file for writing
        std::ofstream export_file(header_output_path, std::ofstream::out | std::ofstream::trunc);
        if (not export_file.is_open()) {
            LOG_ERROR("Filed to open header file for writing")
            exit(4);
        }

        // Add header guard
        export_file << "#pragma once" << std::endl << std::endl;

        // Iterate over exported functions to exclude implemented ones
        for (const auto& [function_name, decorated_function_name] : dll_exports) {
            auto comment = implemented_functions.contains(function_name);

            const String line = fmt::format(
                R"({}#pragma comment(linker, "/export:{}={}.{}"))",
                comment ? "// " : "",
                decorated_function_name,
                forwarded_dll_name,
                decorated_function_name
            );

            export_file << line << std::endl;
        }
    } catch (const Exception& ex) {
        LOG_ERROR("Error: {}", ex.what())
        exit(-1);
    }
}