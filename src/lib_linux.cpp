#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

// This must be before linux headers to avoid name collisions
#include <elfio/elfio.hpp>

#include <dlfcn.h>
#include <fcntl.h>
#include <link.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/stat.h>

#include "koalabox/lib.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/path.hpp"

namespace koalabox::lib {
    using namespace koalabox::lib;
    namespace fs = std::filesystem;

    fs::path get_fs_path(void* const module_handle) {
        char path[PATH_MAX]{};

        if(!module_handle) {
            // Get path to the current executable
            const auto len = readlink("/proc/self/exe", path, sizeof(path) - 1);
            if(len != -1) {
                path[len] = '\0';
                return path::from_str(path);
            }
        } else {
            link_map* lm;
            if(dlinfo(module_handle, RTLD_DI_LINKMAP, &lm) == 0) { // NOLINT(*-multi-level-implicit-pointer-conversion)
                return path::from_str(lm->l_name);
            }
        }

        throw std::runtime_error("Failed to get path from module handle");
    }

    std::optional<void*> get_function_address(void* const lib_handle, const char* function_name) {
        if(auto* const address = dlsym(lib_handle, function_name)) {
            return {address};
        }

        return {};
    }

    std::optional<section_t> get_section(void* lib_handle, const std::string& section_name) {
        // TODO: Check if this can be done using elfio library
        // TODO: Inspect this code carefully
        link_map* lm;
        dlinfo(lib_handle, RTLD_DI_LINKMAP, &lm); // NOLINT(*-multi-level-implicit-pointer-conversion)

        struct context_t {
            const std::string section_name;
            const void* module_base{};
            const link_map* link_map = nullptr;
            std::optional<section_t> result;
        };
        context_t initial_context{section_name, lib_handle, lm, {}};

        dl_iterate_phdr(
            // ReSharper disable once CppParameterMayBeConstPtrOrRef
            [](dl_phdr_info* const header_info, [[maybe_unused]] size_t size, void* context_raw) {
                static constexpr auto CONTINUE_ITERATION = 0;
                static constexpr auto STOP_ITERATION = 1;

                auto* const context = static_cast<context_t*>(context_raw);

                // Check if this is the module we're looking for
                if(header_info->dlpi_addr != context->link_map->l_addr) {
                    return CONTINUE_ITERATION;
                }

                // Open the ELF file
                auto* elf_file = fopen(header_info->dlpi_name, "rb");
                if(!elf_file) {
                    LOG_ERROR("Failed to open library: {}", header_info->dlpi_name);
                    return STOP_ITERATION;
                }

                const auto cleanup = [&] {
                    fclose(elf_file);
                    return STOP_ITERATION;
                };

                // Read the ELF header
                ElfW(Ehdr) elf_header;
                if(fread(&elf_header, 1, sizeof(elf_header), elf_file) != sizeof(elf_header)) {
                    LOG_ERROR("Failed to read elf header: {}", header_info->dlpi_name);
                    return cleanup();
                }

                // Validate ELF magic number
                if(memcmp(elf_header.e_ident, ELFMAG, SELFMAG) != 0) {
                    LOG_ERROR("Not a valid ELF file: {}", header_info->dlpi_name);
                    return cleanup();
                }

                // Read section headers
                ElfW(Shdr) section_header;
                if(fseek(
                       elf_file,
                       elf_header.e_shoff + (elf_header.e_shstrndx * sizeof(section_header)),
                       SEEK_SET
                   ) != 0) {
                    LOG_ERROR("Failed to seek to section header: {}", header_info->dlpi_name);
                    return cleanup();
                }
                if(fread(&section_header, 1, sizeof(section_header), elf_file) != sizeof(section_header)) {
                    LOG_ERROR("Failed to read section header: {}", header_info->dlpi_name);
                    return cleanup();
                };

                // Allocate memory for section names
                auto* section_names = static_cast<char*>(malloc(section_header.sh_size));
                if(!section_names) {
                    LOG_ERROR("Failed to allocate memory for section names");
                    return cleanup();
                }

                if(fseek(elf_file, section_header.sh_offset, SEEK_SET) != 0) { // NOLINT(*-narrowing-conversions)
                    LOG_ERROR("Failed to seek to section names: {}", header_info->dlpi_name);
                    return cleanup();
                }
                if(fread(section_names, 1, section_header.sh_size, elf_file) != section_header.sh_size) {
                    LOG_ERROR("Failed to read section names: {}", header_info->dlpi_name);
                    return cleanup();
                }

                // Iterate through sections
                for(int i = 0; i < elf_header.e_shnum; i++) {
                    if(fseek(elf_file, elf_header.e_shoff + (i * sizeof(section_header)), SEEK_SET) != 0) {
                        LOG_ERROR("Failed to seek to section header: {}", header_info->dlpi_name);
                        return cleanup();
                    }
                    if(fread(&section_header, 1, sizeof(section_header), elf_file) != sizeof(section_header)) {
                        LOG_ERROR("Failed to read to section header: {}", header_info->dlpi_name);
                        return cleanup();
                    };

                    const char* name = section_names + section_header.sh_name;
                    if(name == context->section_name) {
                        const auto section_size = section_header.sh_size;
                        auto* start = reinterpret_cast<uint8_t*>(header_info->dlpi_addr + section_header.sh_offset);
                        auto* end = start + section_size;

                        context->result = section_t{
                            .start_address = start,
                            .end_address = end,
                            .size = static_cast<uint32_t>(section_size)
                        };

                        break;
                    }
                }

                free(section_names);
                return cleanup();
            }, &initial_context
        );

        return initial_context.result;
    }

    std::optional<void*> load_library(const fs::path& library_path) {
        LOG_DEBUG("Loading library: '{}'", path::to_str(library_path));

        // RTLD_LOCAL here is very important, it avoids unintended overwrites of function exports.
        if(auto* library = dlopen(path::to_str(library_path).c_str(), RTLD_NOW | RTLD_LOCAL)) {
            return {library};
        }

        LOG_ERROR("Failed to load library: {}", dlerror());
        return {};
    }

    void unload_library(void* library_handle) {
        dlclose(library_handle);
    }

    void* get_library_handle(const std::string& library_name) {
        const auto full_lib_name = std::format("{}.so", library_name);
        return dlopen(full_lib_name.c_str(), RTLD_NOW | RTLD_GLOBAL);
    }

    std::optional<exports_t> get_exports(void* const lib_handle) {
        const auto lib_path_str = path::to_str(get_fs_path(lib_handle));

        ELFIO::elfio reader;
        if(!reader.load(lib_path_str)) {
            LOG_ERROR("Failed to read ELF file: {}", lib_path_str);
            return std::nullopt;
        }

        ELFIO::section* dynsym = reader.sections[".dynsym"];
        if(!dynsym) {
            LOG_ERROR("Failed to find dynamic symbol table section: {}", lib_path_str);
            return std::nullopt;
        }

        exports_t results;

        const ELFIO::symbol_section_accessor symbols(reader, dynsym);
        for(auto i = 0U; i < symbols.get_symbols_num(); ++i) {
            std::string name;
            ELFIO::Elf64_Addr value;
            ELFIO::Elf_Xword size;
            unsigned char bind;
            unsigned char type;
            unsigned char other;
            ELFIO::Elf_Half section_index;

            symbols.get_symbol(i, name, value, size, bind, type, section_index, other);

            // Exported functions: global/weak binding, type FUNC, default visibility, non-empty name
            if(
                section_index != SHN_UNDEF && // Exported, not just referenced
                (bind == STB_GLOBAL || bind == STB_WEAK) && // Global or weak binding
                (other & 0x3) == STV_DEFAULT && // Default visibility
                !name.empty()
            ) {
                results.insert(name);
            }
        }

        return results;
    }

    std::optional<Bitness> get_bitness(const fs::path& library_path) {
        ELFIO::elfio reader;
        const auto lib_path_str = path::to_str(library_path);
        if(!reader.load(lib_path_str)) {
            LOG_ERROR("Failed to load ELF file: {}", lib_path_str);
            return std::nullopt;
        }

        switch(reader.get_class()) {
        case ELFCLASS32: return Bitness::$32;
        case ELFCLASS64: return Bitness::$64;
        default:
            LOG_ERROR("Unknown ELF class: {}", reader.get_class());
            return std::nullopt;
        }
    }
}
