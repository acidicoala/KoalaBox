#include <fstream>
#include <set>

#include <cxxopts.hpp>
#include <glob/glob.h>
#include <inja/inja.hpp>

#include <koalabox/logger.hpp>
#include <koalabox/path.hpp>
#include <koalabox/platform.hpp>

#include <koalabox_tools/cmd.hpp>
#include <koalabox_tools/module.hpp>

namespace {
    // language=c++
    constexpr auto HEADER_TEMPLATE = R"(// Auto-generated header file
#pragma once

namespace {{ namespace_id }} {
    void init(void* self_lib_handle, void* original_lib_handle);
}
)";

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

## for symbol_name in exported_symbols
EXPORT void {{ symbol_name }}() {
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
## for symbol_name in exported_symbols

        dest_address = dlsym(self_lib_handle, "{{ symbol_name }}");
        src_address = dlsym(original_lib_handle, "{{ symbol_name }}");
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

        static Args parse(const int argc, const char** argv) {
            KBT_CMD_PARSE_ARGS(
                "linux_exports_generator", "Generates proxy exports for linux libraries",
                argc, argv,
                input_libs_glob,
                output_path
            )

            return args;
        }
    };

    auto get_exported_symbols(const std::string& input_libs_glob) {
        kb::tools::module::exports_t results;

        for(const auto& lib_path : glob::rglob(input_libs_glob)) {
            if(const auto exports = kb::tools::module::get_exports(lib_path)) {
                for(const auto& symbol : *exports) {
                    // TODO: Check if the function is implemented
                    if(!symbol.starts_with("_")) {
                        results.insert(symbol);
                    }
                }
            } else {
                // FIXME: GitHub runner fails to process binaries from SDK version 106
                LOG_WARN("Failed to get module exports: {}", kb::path::to_str(lib_path));
            }
        }

        return results;
    }

    void generate_header_and_source(const std::string& output_path, kb::tools::module::exports_t exports) {
        const auto header_path = kb::path::from_str(output_path + ".hpp");
        const auto source_path = kb::path::from_str(output_path + ".cpp");

        const nlohmann::json context = {
            {"header_filename", kb::path::to_str(header_path.filename())},
            {"namespace_id", kb::path::to_str(header_path.stem())},
            {"exported_symbols", exports},
            {"is_32bit", kb::platform::is_32bit},
#if defined(KB_32)
            {"address_offset", 1}, // movl   takes 1 byte  for opcode + 4 bytes for address
#elif defined(KB_64)
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
    }
}

int main(const int argc, const char* argv[]) {
    try {
        kb::logger::init_console_logger();

        // ReSharper disable once CppUseStructuredBinding
        const auto args = Args::parse(argc, argv);

        const auto exported_symbols = get_exported_symbols(args.input_libs_glob);
        LOG_INFO("Collected {} symbols", exported_symbols.size());

        generate_header_and_source(args.output_path, exported_symbols);
        LOG_INFO("Successfully generated proxy exports");
    } catch(const std::exception& e) {
        LOG_ERROR("Unhandled global exception: {}", e.what());
        exit(EXIT_FAILURE);
    }
}
