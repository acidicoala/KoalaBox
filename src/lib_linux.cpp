#include <cstring>
#include <fstream>
#include <string>
#include <vector>

// ELFIO must be included before linux headers to avoid name collisions
#include <elfio/elfio.hpp>

#include <dlfcn.h>
#include <link.h>
#include <unistd.h>
#include <linux/limits.h>

#include "koalabox/core.hpp"
#include "koalabox/lib.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/path.hpp"

namespace {
    using namespace koalabox::lib;

    std::optional<section_t> read_section(
        const std::string& elf_path,
        const std::string& section_name,
        ElfW(Addr) const lib_base
    ) {
        ELFIO::elfio reader;
        if(!reader.load(elf_path)) {
            LOG_ERROR("Failed to load library in ELFIO: {}", elf_path);
            return std::nullopt;
        }

        for(const auto& sec : reader.sections) {
            if(sec->get_name() == section_name) {
                // Correct section found

                const auto offset = sec->get_offset();
                const auto size = sec->get_size();
                auto* const base = reinterpret_cast<uint8_t*>(lib_base);
                auto* start = base + offset;
                auto* end = start + size;
                return section_t{start, end, static_cast<uint32_t>(size)};
            }
        }

        LOG_ERROR("Failed to find section '{}' in {}", section_name, elf_path);
        return std::nullopt;
    }
}

namespace koalabox::lib {
    namespace fs = std::filesystem;

    fs::path get_fs_path(void* const lib_handle) {
        char path[PATH_MAX]{};

        if(!lib_handle) {
            // Get path to the current executable
            const auto len = readlink("/proc/self/exe", path, sizeof(path) - 1);
            if(len != -1) {
                path[len] = '\0';
                return path::from_str(path);
            }
        } else {
            link_map* lm;
            if(dlinfo(lib_handle, RTLD_DI_LINKMAP, &lm) == 0) { // NOLINT(*-multi-level-implicit-pointer-conversion)
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

                if(info->dlpi_addr == ctx->lm->l_addr) {
                    // Correct library found

                    ctx->result = read_section(info->dlpi_name, ctx->section_name, info->dlpi_addr);

                    return 1; // Stop iteration
                }

                if(info->dlpi_addr == reinterpret_cast<ElfW(Addr)>(ctx->lm)) {
                    // Special case for exe handle
                    const auto exe_path = get_fs_path(nullptr);

                    ctx->result = read_section(path::to_str(exe_path), ctx->section_name, info->dlpi_addr);

                    return 1;
                }

                return 0;
            }, &initial_context
        );

        return initial_context.result;
    }

    std::optional<void*> load(const fs::path& library_path) {
        LOG_DEBUG("Loading library: '{}'", path::to_str(library_path));

        // RTLD_LOCAL here is very important, it avoids unintended overwrites of function exports.
        if(auto* library = dlopen(path::to_str(library_path).c_str(), RTLD_NOW | RTLD_LOCAL)) {
            return {library};
        }

        LOG_ERROR("Failed to load library: {}", dlerror());
        return {};
    }

    void unload(void* lib_handle) {
        dlclose(lib_handle);
    }

    void* get_lib_handle(const std::string& lib_name) {
        struct context_t {
            const std::string& lib_name;
            void* result;
        } initial_context{lib_name, nullptr};

        dl_iterate_phdr(
            [](dl_phdr_info* const info, size_t, void* ctx) {
                auto* context = static_cast<context_t*>(ctx);
                const auto current_name = path::to_str(path::from_str(info->dlpi_name).stem());

                // current_name might end with version, like .so.1, so we have to check the start
                if(current_name.starts_with(context->lib_name)) {
                    context->result = dlopen(info->dlpi_name, RTLD_NOW | RTLD_NOLOAD);
                    return 1;
                }

                return 0; // Continue
            }, &initial_context
        );

        return initial_context.result;
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

    void* get_exe_handle() {
        // This doesn't return actual exe handle: dlopen(nullptr, RTLD_NOW);

        std::ifstream maps(std::format("/proc/{}/maps", getpid()));
        if(!maps) {
            LOG_ERROR("Failed to open self maps")
            return nullptr;
        }

        std::string line;
        if(!std::getline(maps, line)) {
            LOG_ERROR("Failed to read line from self maps")
            return nullptr;
        }

        const auto hex_str = line.substr(0, sizeof(uintptr_t) * 2);
        LOG_TRACE("{} -> first line: {}", __func__, line);

        const auto start_addr = std::stoul(hex_str, nullptr, 16);
        LOG_TRACE("{} -> start addr: {:#x}", __func__, start_addr);

        return reinterpret_cast<void*>(start_addr);
    }
}
