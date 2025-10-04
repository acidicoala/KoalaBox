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

#include "koalabox/core.hpp"
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
        link_map* lm;
        if(dlinfo(lib_handle, RTLD_DI_LINKMAP, &lm) != 0) { // NOLINT(*-multi-level-implicit-pointer-conversion)
            LOG_ERROR("Failed to get link_map from lib handle: {}", lib_handle);
            return std::nullopt;
        }

        // Find the correct library
        struct context_t {
            const std::string& section_name;
            const link_map* lm = nullptr;
            std::optional<section_t> result;
        } initial_context{section_name, lm, {}};

        dl_iterate_phdr(
            [](dl_phdr_info* const info, size_t, void* data) {
                auto* ctx = static_cast<context_t*>(data);
                if(info->dlpi_addr != ctx->lm->l_addr) {
                    return 0;
                }

                // Correct library found

                ELFIO::elfio reader;
                if(!reader.load(info->dlpi_name)) {
                    LOG_ERROR("Failed to load library in ELFIO: {}", info->dlpi_name);
                    return 1;
                }
                for(const auto& sec : reader.sections) {
                    if(sec->get_name() == ctx->section_name) {
                        // Correct section found

                        const auto offset = sec->get_offset();
                        const auto size = sec->get_size();
                        auto* const base = reinterpret_cast<uint8_t*>(info->dlpi_addr);
                        auto* start = base + offset;
                        auto* end = start + size;
                        ctx->result = section_t{start, end, static_cast<uint32_t>(size)};
                        break;
                    }
                }
                return 1; // Stop iteration
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

    std::optional<exports_t> get_exports(const fs::path& lib_path) {
        const auto lib_path_str = path::to_str(lib_path);

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

    exports_t get_exports_or_throw(const fs::path& lib_path) {
        return get_exports(lib_path) | throw_if_empty(
                   std::format("Failed get library exports of {}", path::to_str(lib_path))
               );
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
