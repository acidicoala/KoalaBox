#include "koalabox/koalabox.hpp"
#include "koalabox/util/util.hpp"
#include "koalabox/win_util/win_util.hpp"
#include "koalabox/loader/loader.hpp"

// Ensure that assert macros work
#include <cassert>

#undef NDEBUG

#include "spdlog/sinks/stdout_sinks.h"

using namespace koalabox;
using namespace std::filesystem;
using std::ofstream;
using std::endl;

Set<String> get_implemented_functions(const Path& path);

/**
 * Args <br>
 * 0: program name <br>
 * 1: forwarded dll name <br>
 * 2: dll input path <br>
 * 3: sources input path <br>
 * 4: header output path <br>
 */
int wmain(const int argc, const wchar_t* argv[]) {
    logger = spdlog::stdout_logger_st("stdout");

    assert(argc == 4 || argc == 5);

    String forwarded_dll_name(util::to_string(argv[1]));
    Path dll_input_path(argv[2]);
    Path header_output_path(argv[3]);
    Path sources_input_path(argc == 5 ? argv[4] : L"");

    assert(exists(dll_input_path));

    auto implemented_functions = sources_input_path.empty()
        ? Set<String>()
        : get_implemented_functions(sources_input_path);

    const auto library = win_util::load_library(dll_input_path);

    auto exported_functions = loader::get_undecorated_function_map(library);

    // Create directories for export header, if necessary
    create_directories(header_output_path.parent_path());

    // Open the export header file for writing
    ofstream export_file(header_output_path, ofstream::out | ofstream::trunc);
    assert(export_file.is_open());

    // Add header guard
    export_file << "#pragma once" << endl << endl;

    // Iterate over exported functions to exclude implemented ones
    for (const auto&[function_name, mangled_name]: exported_functions) {
        // On x64 architecture, mangled_name is equal to function_name
        if (implemented_functions.contains(function_name)) {
            export_file << "// ";
        }

        export_file << "#pragma comment(linker, " << '"' << "/export:" << mangled_name << '='
                    << forwarded_dll_name << '.' << mangled_name << '"' << ')' << endl;
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
                logger->debug("Implemented function: {}", func_name);
            }

            file_content = match.suffix();
        }
    }

    return implemented_functions;
}
