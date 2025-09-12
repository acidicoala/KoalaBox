#include <fstream>
#include <iostream>
#include <regex>
#include <set>

#include <glob/glob.h>

#include <koalabox/io.hpp>
#include <koalabox/loader.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/parser.hpp>
#include <koalabox/path.hpp>
#include <koalabox/str.hpp>

#include "koalabox/module.hpp"

namespace {
    namespace kb = koalabox;
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

    bool parseBoolean(const std::string& bool_str) {
        if(kb::str::eq(bool_str, "TRUE")) {
            return true;
        }

        if(kb::str::eq(bool_str, "FALSE")) {
            return false;
        }

        LOG_ERROR("Invalid boolean value: {}", bool_str);
        exit(10);
    }

    auto get_library_exports_map(const std::string& lib_files_glob, const bool undecorate) {
        std::map<std::string, std::string> dll_exports;

        const auto lib_path_list = glob::glob(lib_files_glob);
        LOG_INFO("Found {} library files", lib_path_list.size());

        for(const auto& lib_path : lib_path_list) {
            if(not fs::exists(lib_path)) {
                continue;
            }

            auto* const library = kb::module::load_library_or_throw(lib_path);
            const auto lib_exports = kb::loader::get_export_map(library, undecorate);

            dll_exports.insert(lib_exports.begin(), lib_exports.end());
        }

        LOG_INFO("Found {} exported functions", dll_exports.size());

        return dll_exports;
    }

    kb::module::exports_t get_library_exports(const std::string& lib_files_glob) {
        kb::module::exports_t all_exports;

        const auto lib_path_list = glob::glob(lib_files_glob);
        LOG_INFO("Found {} library files", lib_path_list.size());

        for(const auto& lib_path : lib_path_list) {
            if(not fs::exists(lib_path)) {
                continue;
            }

            const auto lib_exports = kb::module::get_exports(lib_path);
            all_exports.insert(lib_exports.begin(), lib_exports.end());
        }

        LOG_INFO("Found {} exported functions", all_exports.size());

        return all_exports;
    }

    // Used for windows
    void generate_linker_exports_header(
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

    // Used for linux
    void generate_proxy_wrappers_source(
        std::ofstream& export_file,
        const kb::module::exports_t& lib_exports,
        const std::set<std::string>& defined_functions,
        const std::string& forwarded_dll_name
    ) {
        // language=c++
        const auto prologue = std::format(
            R"(
#include <dlfcn.h>

extern "C" void push_all();
extern "C" void pop_all();

#define EXPORT __attribute__((visibility("default")))

namespace {{
    using void_fn = void(*)();

    void_fn find(const char* name) {{
        static auto* module_handle = dlopen("{}", RTLD_NOW);
        return reinterpret_cast<void_fn>(dlsym(module_handle, name));
    }}
}}
)", forwarded_dll_name
        );

        export_file << prologue;

        for(const auto& function_name : lib_exports) {
            // Comment out exports that we have defined
            const auto declaration = std::format(
                // language=c++
                R"(EXPORT void {}())", function_name
            );

            export_file << std::endl;

            if(defined_functions.contains(function_name)) {
                export_file << "// " << declaration << ";" << std::endl;
                continue;
            }

            export_file
                << declaration << " {" << std::endl
                << "    push_all();" << std::endl
                << "    static const auto func = find(__func__);" << std::endl
                << "    pop_all();" << std::endl
                << "    func();" << std::endl
                << "}" << std::endl;
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

        for(int i = 0; i < argc; i++) {
            LOG_INFO("Arg #{} = '{}'", i, koalabox::str::to_str(argv[i]));
        }

        if(argc != 5 && argc != 6) {
            LOG_ERROR("Invalid number of arguments. Expected 5 or 6. Got: {}", argc);
            exit(1);
        }

        const auto undecorate = parseBoolean(kb::str::to_str(argv[1]));
        const auto forwarded_dll_name = kb::str::to_str(argv[2]);
        const auto lib_files_glob = kb::str::to_str(argv[3]);
        const auto output_file_path = fs::path(kb::str::to_str(argv[4]));

        // Input sources are optional because Koaloader doesn't have them.
        const auto sources_input_path = fs::path(argc == 6 ? kb::str::to_str(argv[5]) : "");

        const auto defined_functions = sources_input_path.empty()
                                           ? std::set<std::string>()
                                           : get_defined_functions(sources_input_path);


        // Create directories for export file, if necessary
        fs::create_directories(output_file_path.parent_path());

        // Open the export header file for writing
        std::ofstream export_file(output_file_path, std::ofstream::out | std::ofstream::trunc);
        if(not export_file.is_open()) {
            LOG_ERROR("Filed to open header file for writing");
            exit(4);
        }

#ifdef _WIN32
        const auto lib_exports = get_library_exports_map(lib_files_glob, undecorate);
        generate_linker_exports_header(export_file, lib_exports, defined_functions, forwarded_dll_name);
#else
        const auto lib_exports = get_library_exports(lib_files_glob);
        generate_proxy_wrappers_source(export_file, lib_exports, defined_functions, forwarded_dll_name);
#endif

        LOG_INFO("Finished generating {}", kb::path::to_str(output_file_path));
    } catch(const std::exception& ex) {
        LOG_ERROR("Error: {}", ex.what());
        exit(-1);
    }

    koalabox::logger::shutdown();
    return 0;
}
