#include <fstream>
#include <regex>
#include <set>

#include <cxxopts.hpp>
#include <glob/glob.h>
#include <inja/inja.hpp>

#include <koalabox/io.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/path.hpp>

#include <koalabox_tools/cmd.hpp>
#include <koalabox_tools/module.hpp>
#include <koalabox_tools/parser.hpp>

namespace {
    // language=c++
    constexpr auto* HEADER_TEMPLATE =R"(// Auto-generated header file
#pragma once

## for function_name in function_names
{% if function_name in implemented_functions %}{% set comment="//" %}{% else %}{% set comment="" %}{% endif %}
{{ comment }}#pragma comment(linker, "/export:{{ function_name }}={{ forwarded_dll_name }}.{{ function_name }}")
## endfor

)";

    namespace kb = koalabox;
    namespace fs = std::filesystem;

    struct Args {
        std::string forwarded_dll_name;
        std::string lib_files_glob;
        std::string output_file_path;
        std::string sources_input_path;

        static Args parse(const int argc, const TCHAR** argv) {
            const auto normalized = kb::tools::cmd::normalize_args(argc, argv);

            KBT_CMD_PARSE_ARGS(
                "linux_exports_generator", "Generates proxy exports for linux libraries",
                argc, normalized.argv.data(),
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
    auto get_implemented_functions(const fs::path& path) {
        std::set<std::string> declared_functions;

        for(const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            const auto& file_path = entry.path();
            if(file_path.extension() != ".cpp") {
                continue;
            }

            LOG_DEBUG("Processing source file: {}", kb::path::to_str(file_path));

            const auto file_content = koalabox::io::read_file(file_path);

            const auto processed_source = preprocess_source_file(file_content);

            const auto query_results = koalabox::tools::parser::query(
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

    auto get_library_exports_map(const std::string& lib_files_glob) {
        kb::tools::module::exports_t all_exports;

        const auto lib_path_list = glob::rglob(lib_files_glob);
        LOG_INFO("Found {} library file(s)", lib_path_list.size());

        for(const auto& lib_path : lib_path_list) {
            const auto exports = kb::tools::module::get_exports_or_throw(lib_path);

            all_exports.insert(exports.begin(), exports.end());
        }

        LOG_INFO("Found {} exported functions", all_exports.size());

        return all_exports;
    }
}

int MAIN(const int argc, const TCHAR* argv[]) { // NOLINT(*-use-internal-linkage)
    try {
        kb::logger::init_console_logger();

        const auto args = Args::parse(argc, argv);

        const auto output_file_path = kb::path::from_str(args.output_file_path);

        // Input sources are optional because SmokeAPI and Koaloader don't have them.
        const auto sources_input_path = kb::path::from_str(args.sources_input_path);

        const auto implemented_functions = sources_input_path.empty()
                                           ? std::set<std::string>()
                                           : get_implemented_functions(sources_input_path);

        // Create directories for export file, if necessary
        fs::create_directories(output_file_path.parent_path());

        const auto lib_exports = get_library_exports_map(args.lib_files_glob);

        const nlohmann::json context = {
            {"forwarded_dll_name", args.forwarded_dll_name},
            {"function_names", lib_exports},
            {"implemented_functions", implemented_functions},
        };

        if(std::ofstream header_file(output_file_path); header_file.is_open()) {
            inja::Environment env;
            env.set_trim_blocks(true);
            env.set_lstrip_blocks(true);
            env.render_to(header_file, env.parse(HEADER_TEMPLATE), context);
        } else {
            throw std::runtime_error("Could not open header file for writing");
        }

        LOG_INFO("Finished generating {}", kb::path::to_str(output_file_path));
    } catch(const std::exception& ex) {
        LOG_ERROR("Error: {}", ex.what());
        exit(EXIT_FAILURE);
    }

    koalabox::logger::shutdown();
    return EXIT_SUCCESS;
}
