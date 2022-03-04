#include "koalabox/koalabox.hpp"
#include "koalabox/util/util.hpp"
#include "koalabox/win_util/win_util.hpp"
#include "koalabox/loader/loader.hpp"

#include "spdlog/sinks/stdout_sinks.h"

using namespace koalabox;
using namespace std::filesystem;
using std::ofstream;
using std::endl;

bool parseBoolean(const String& string);

Set<String> get_implemented_functions(const Path& path);

/**
 * Args <br>
 * 0: program name <br>
 * 1: undecorate? <br>
 * 2: forwarded dll name <br>
 * 3: dll input path <br>
 * 4: sources input path <br>
 * 5: header output path <br>
 */
int wmain(const int argc, const wchar_t* argv[]) {
    logger = spdlog::stdout_logger_st("stdout");
    logger->flush_on(spdlog::level::trace);

    if (argc < 5 || argc > 6) {
        logger->error("Invalid number of arguments. Expected 5 or 6. Got: {}", argc);
        exit(1);
    }

    auto undecorate = parseBoolean(util::to_string(argv[1]));
    auto forwarded_dll_name = util::to_string(argv[2]);
    auto dll_input_path = Path(argv[3]);
    auto header_output_path = Path(argv[4]);
    auto sources_input_path = Path(argc == 6 ? argv[5] : L""); // Optional for Koaloader

    if (not exists(dll_input_path)) {
        logger->error("Non-existent DLL path: {}", dll_input_path.string());
        exit(2);
    }

    auto implemented_functions = sources_input_path.empty()
        ? Set<String>()
        : get_implemented_functions(sources_input_path);

    const auto library = win_util::load_library_or_throw(dll_input_path);

    auto exported_functions = loader::get_export_map(library, undecorate);

    // Create directories for export header, if necessary
    create_directories(header_output_path.parent_path());

    // Open the export header file for writing
    ofstream export_file(header_output_path, ofstream::out | ofstream::trunc);
    if (not export_file.is_open()) {
        logger->error("Filed to open header file for writing");
        exit(3);
    }

    // Add header guard
    export_file << "#pragma once" << endl << endl;

    // Iterate over exported functions to exclude implemented ones
    for (const auto&[function_name, decorated_function_name]: exported_functions) {
        auto comment = implemented_functions.contains(function_name);

        String line = fmt::format(
            R"({}#pragma comment(linker, "/export:{}={}.{}"))",
            comment ? "// " : "",
            decorated_function_name,
            forwarded_dll_name,
            decorated_function_name
        );

        export_file << line << endl;
    }
}

/**
 * Returns a list of functions parsed from the sources
 * in a given directory. Edge cases: Comments
 */
Set<String> get_implemented_functions(const Path& path) {
    Set<String> implemented_functions;

    for (auto& p: recursive_directory_iterator(path)) {
        const auto file_path = p.path();

        std::ifstream ifs(file_path);
        std::string file_content(std::istreambuf_iterator<char>{ ifs }, {});

        // Matches function name in the 1st group
        static const std::regex func_name_pattern(R"(\s*DLL_EXPORT\([\w|\s]+\**\)\s*(\w+)\s*\()");
        std::smatch match;
        while (regex_search(file_content, match, func_name_pattern)) {
            const auto func_name = match.str(1);

            if (not implemented_functions.contains(func_name)) {
                implemented_functions.insert(func_name);
                logger->info("Implemented function: \"{}\"", func_name);
            }

            file_content = match.suffix();
        }
    }

    return implemented_functions;
}

bool parseBoolean(const String& string) {
    if (util::strings_are_equal(string, "true")) {
        return true;
    } else if (util::strings_are_equal(string, "false")) {
        return false;
    } else {
        logger->error("Invalid boolean value: {}", string);
        exit(10);
    }
}
