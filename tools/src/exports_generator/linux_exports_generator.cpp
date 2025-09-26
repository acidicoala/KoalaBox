#include <fstream>
#include <set>

#include <cxxopts.hpp>
#include <glob/glob.h>
#include <inja/inja.hpp>

#include <koalabox/lib.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/path.hpp>

namespace {
    // language=c++
    constexpr auto HEADER_TEMPLATE = R"(#pragma once

namespace {{ namespace_id }} {
    void init(void* self_lib_handle, void* original_lib_handle);
}
)";

    // TODO: 32-bit support
    // TODO: Fallback function when dlsym is null
    // language=c++
    constexpr auto SOURCE_TEMPLATE = R"(// Auto-generated source file
#include <cstring>
#include <dlfcn.h>

#include <polyhook2/MemProtector.hpp>

#include <koalabox/lib.hpp>

#include "{{ header_filename }}"

#define EXPORT extern "C" __attribute__((visibility("default"))) __attribute__((naked))

## for function_name in function_names
EXPORT void {{ function_name }}() {
    asm volatile ("movabs $0xBaadF00dDeadBeef, %%rax":::"rax");
    asm volatile ("jmp *%rax");
}

## endfor

namespace {{ namespace_id }} {
    void init(void* const self_lib_handle, void* const original_lib_handle) {
        const auto code_section = koalabox::lib::get_section_or_throw(self_lib_handle, koalabox::lib::CODE_SECTION);
        PLH::MemAccessor mem_accessor;
        PLH::MemoryProtector const protector(
            reinterpret_cast<uint64_t>(code_section.start_address),
            code_section.size,
            PLH::ProtFlag::RWX,
            mem_accessor
        );

        void* address = nullptr;
## for function_name in function_names

        address = dlsym(original_lib_handle, "{{ function_name }}");
        std::memcpy(reinterpret_cast<uint8_t*>(&{{ function_name }}) + 2, &address, sizeof(void*));
## endfor
    }
}
)";

    namespace kb = koalabox;

    struct Args {
        std::string input_libs_glob;
        std::string output_path;
    };

    Args parse_args(const int argc, const char** argv) {
        const std::string INPUT_LIBS_GLOB = "input_libs_glob";
        const std::string OUTPUT_PATH = "output_path";

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
}

int main(const int argc, const char* argv[]) {
    try {
        kb::logger::init_console_logger();

        // ReSharper disable once CppUseStructuredBinding
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

        const auto header_path = kb::path::from_str(args.output_path + ".hpp");
        const auto source_path = kb::path::from_str(args.output_path + ".cpp");

        const nlohmann::json context = {
            {"header_filename", kb::path::to_str(header_path.filename())},
            {"namespace_id", kb::path::to_str(header_path.stem())},
            {"function_names", function_names}
        };

        if(std::ofstream header_file(header_path); header_file.is_open()) {
            inja::render_to(header_file, HEADER_TEMPLATE, context);
        } else {
            throw std::runtime_error("Could not open header file for writing");
        }

        if(std::ofstream source_file(source_path); source_file.is_open()) {
            inja::render_to(source_file, SOURCE_TEMPLATE, context);
        } else {
            throw std::runtime_error("Could not open header file for writing");
        }
    } catch(const std::exception& e) {
        LOG_ERROR("Unhandled global exception: {}", e.what());
        exit(EXIT_FAILURE);
    }
}
