#include <fstream>
#include <set>

#include <cxxopts.hpp>
#include <glob/glob.h>
#include <inja/inja.hpp>

#include <koalabox/lib.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/path.hpp>
#include <koalabox/platform.hpp>

namespace {
    // language=c++
    constexpr auto HEADER_TEMPLATE = R"(// Auto-generated header file
#pragma once

namespace {{ namespace_id }} {
    void init(void* self_lib_handle, void* original_lib_handle);
}
)";

    // TODO: Fallback function when dlsym is null
    // language=c++
    constexpr auto SOURCE_TEMPLATE = R"(// Auto-generated source file
#include <cstring>
#include <dlfcn.h>
#include <mutex>

#include <polyhook2/MemProtector.hpp>

#include <koalabox/lib.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/util.hpp>

#include "{{ header_filename }}"

#define EXPORT extern "C" __attribute__((visibility("default"))) __attribute__((naked))

## for function_name in function_names
EXPORT void {{ function_name }}() {
{% if is_32bit %}
    asm volatile ("movl $0xDeadC0de, %%eax":::"eax");
    asm volatile ("jmp *%eax");
{% else %}
    asm volatile ("movabs $0xFeedBeefDeadC0de, %%rax":::"rax");
    asm volatile ("jmp *%rax");
{% endif %}
}

## endfor

namespace {
    void panic_exit(){
        koalabox::util::panic("Invocation of uninitialized exported function.");
    }
}

namespace {{ namespace_id }} {
    void init(void* const self_lib_handle, void* const original_lib_handle) {
        static std::mutex section;
        const std::lock_guard lock(section);

        static bool initialized = false;
        if(initialized) {
            LOG_WARN("{{ namespace_id }} is already initialized");
            return;
        }

        LOG_INFO(
            "Initializing {{ namespace_id }}. self handle: {}, original handle: {}",
            self_lib_handle, original_lib_handle
        );

        const auto code_section = koalabox::lib::get_section_or_throw(self_lib_handle, koalabox::lib::CODE_SECTION);
        PLH::MemAccessor mem_accessor;
        PLH::MemoryProtector const protector(
            reinterpret_cast<uint64_t>(code_section.start_address),
            code_section.size,
            PLH::ProtFlag::RWX,
            mem_accessor
        );

        void* src_address = nullptr;
        void* dest_address = nullptr;
## for function_name in function_names

        dest_address = dlsym(self_lib_handle, "{{ function_name }}");
        src_address = dlsym(original_lib_handle, "{{ function_name }}");
        if(!src_address) src_address = reinterpret_cast<void*>(panic_exit);
        std::memcpy(static_cast<uint8_t*>(dest_address) + {{ address_offset }}, &src_address, sizeof(void*));
## endfor

        initialized = true;
        LOG_INFO("Proxy exports initialized");
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
            {"function_names", function_names},
            {"is_32bit", kb::platform::is_32bit},
#ifdef KB_32
            {"address_offset", 1}, // movl   takes 1 byte  for opcode + 4 bytes for address
#elifdef KB_64
            {"address_offset", 2}, // movabs takes 2 bytes for opcode + 8 bytes for address
#endif
        };

        inja::Environment env;
        env.set_trim_blocks(true);
        env.set_lstrip_blocks(true);

        if(std::ofstream header_file(header_path); header_file.is_open()) {
            env.render_to(header_file, env.parse(HEADER_TEMPLATE), context);
        } else {
            throw std::runtime_error("Could not open header file for writing");
        }

        if(std::ofstream source_file(source_path); source_file.is_open()) {
            env.render_to(source_file, env.parse(SOURCE_TEMPLATE), context);
        } else {
            throw std::runtime_error("Could not open source file for writing");
        }
    } catch(const std::exception& e) {
        LOG_ERROR("Unhandled global exception: {}", e.what());
        exit(EXIT_FAILURE);
    }
}
