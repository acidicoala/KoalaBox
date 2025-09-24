#include <fstream>
#include <set>

#include <cxxopts.hpp>
#include <glob/glob.h>

#include <koalabox/lib.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/path.hpp>

namespace {
    namespace kb = koalabox;
    namespace fs = std::filesystem;

    const std::string INPUT_LIBS_GLOB = "input_libs_glob";
    const std::string OUTPUT_PATH = "output_path";

    struct Args {
        std::string input_libs_glob;
        std::string output_path;
    };

    Args parse_args(const int argc, const char** argv) {
        try {
            for(auto i = 0; i < argc; ++i) {
                LOG_DEBUG("argv[{}] = {}", i, argv[i]);
            }

            cxxopts::Options options("linux_exports_generator", "Generates proxy exports for linux libraries");
            options.add_options()
                (
                    "ilg," + INPUT_LIBS_GLOB, "Libs from which symbol exports will be proxied",
                    cxxopts::value<std::string>()
                )
                ("out," + OUTPUT_PATH, "Output file without extension", cxxopts::value<std::string>());

            const auto args = options.parse(argc, argv);

            const auto input_libs_glob = args[INPUT_LIBS_GLOB].as<std::string>();
            const auto output_path = args[OUTPUT_PATH].as<std::string>();

            LOG_INFO("{:<20} = {}", INPUT_LIBS_GLOB, input_libs_glob);
            LOG_INFO("{:<20} = {}", OUTPUT_PATH, output_path);

            return {input_libs_glob, output_path};
        } catch(const std::exception& e) {
            LOG_ERROR("Error parsing args: {}", e.what());
            exit(EXIT_FAILURE);
        }
    }

    void generate_header(const fs::path& header_path) {
        std::ofstream header_file(header_path);
        if(!header_file.is_open()) {
            throw std::runtime_error("Could not open header file for writing");
        }

        header_file
            << "#pragma once" << std::endl << std::endl
            << "namespace proxy_exports {" << std::endl
            << "    void init(void* original_lib_handle);" << std::endl
            << "}" << std::endl;
    }

    void generate_source(
        const std::set<std::string>& function_names,
        const std::string& header_name,
        const fs::path& source_path
    ) {
        std::ofstream source_file(source_path);
        if(!source_file.is_open()) {
            throw std::runtime_error("Could not open source file for writing");
        }

        source_file // language=c++
            << "#include <dlfcn.h>" << std::endl // language=c++
            << "#include <cstring>" << std::endl << std::endl // language=c++
            << "#include <" << header_name << ">" << std::endl << std::endl // language=c++
            << R"(#define EXPORT extern "C" __attribute__((visibility("default"))) __attribute__((naked)))"
            << std::endl;

        for(const auto& fn_name : function_names) {
            // TODO: 32-bit support
            source_file
                << std::endl
                << "EXPORT void " << fn_name << "() {" << std::endl
                << "    asm volatile (\"movabs $0x" << std::hex << 0xBaadFeedDeadBeef << R"(, %%rax":::"rax");)"
                << std::endl
                << "    asm volatile (\"jmp *%rax\");" << std::endl
                << "}" << std::endl;
        }

        source_file << std::endl // language=c++
            << "namespace proxy_exports {" << std::endl // language=c++
            << "    void init(void* original_lib_handle){" << std::endl // language=c++
            << "        void* address = nullptr;" << std::endl;

        for(const auto& fn_name : function_names) {
            // TODO: Use fallback function if dlsym result is null
            source_file << std::endl
                << "        address = dlsym(original_lib_handle, \"" << fn_name << "\");" << std::endl
                << "        std::memcpy(reinterpret_cast<uint8_t*>(&" << fn_name << ") + 2, &address, sizeof(void*));"
                << std::endl;
        }

        source_file << "    }" << std::endl
            << "}" << std::endl;
    }
}

int main(const int argc, const char* argv[]) {
    try {
        kb::logger::init_console_logger();

        const auto args = parse_args(argc, argv);

        std::set<std::string> function_names;
        for(const auto& lib_path : glob::rglob(args.input_libs_glob)) {
            const auto* lib_handle = kb::lib::load_library_or_throw(lib_path);
            const auto export_map = kb::lib::get_export_map(lib_handle);

            for(const auto& fn_name : export_map | std::views::keys) {
                // TODO: Check if the function is implemented
                function_names.insert(fn_name);
            }
        }

        const auto header_path = args.output_path + ".hpp";
        generate_header(header_path);
        generate_source(function_names, kb::path::from_str(header_path).filename(), args.output_path + ".cpp");
    } catch(const std::exception& e) {
        LOG_ERROR("Unhandled global exception: {}", e.what());
        exit(EXIT_FAILURE);
    }
}
