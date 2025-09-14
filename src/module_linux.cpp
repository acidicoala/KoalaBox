#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <link.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "koalabox/logger.hpp"
#include "koalabox/module.hpp"
#include "koalabox/path.hpp"

namespace {
    using namespace koalabox::module;

    exports_t list_dynsym_exports(const void* map) {
        const auto map_base = static_cast<const char*>(map);
        // TODO: refactor
        const auto* const ehdr = static_cast<const ElfW(Ehdr)*>(map);
        const auto* shdrs = reinterpret_cast<const ElfW(Shdr)*>(map_base + ehdr->e_shoff);
        const auto* shstrtab = map_base + shdrs[ehdr->e_shstrndx].sh_offset;

        const ElfW(Shdr)* dynsym = nullptr;
        const ElfW(Shdr)* dynstr = nullptr;
        for(int i = 0; i < ehdr->e_shnum; ++i) {
            const char* name = shstrtab + shdrs[i].sh_name;
            if(strcmp(name, ".dynsym") == 0) {
                dynsym = &shdrs[i];
            } else if(strcmp(name, ".dynstr") == 0) {
                dynstr = &shdrs[i];
            }
        }
        if(!dynsym || !dynstr) {
            LOG_ERROR(".dynsym or .dynstr not found");
            return {};
        }

        const auto* syms = reinterpret_cast<const ElfW(Sym)*>(map_base + dynsym->sh_offset);
        const auto nsyms = dynsym->sh_size / sizeof(ElfW(Sym));
        const auto* strtab = map_base + dynstr->sh_offset;

        exports_t exports;
        for(size_t i = 0; i < nsyms; ++i) {
            const ElfW(Sym)& s = syms[i];
            // Both Elf32_Sym and Elf64_Sym use the same one-byte st_info field.
            if(ELF64_ST_TYPE(s.st_info) == STT_FUNC &&
               (ELF64_ST_BIND(s.st_info) == STB_GLOBAL || ELF64_ST_BIND(s.st_info) == STB_WEAK) &&
               s.st_name != 0 &&
               s.st_shndx != SHN_UNDEF) {
                const std::string function_name = strtab + s.st_name;
                if(!function_name.starts_with("_")) {
                    exports.insert(function_name);
                }
            }
        }
        return exports;
    }
}

namespace koalabox::module {
    namespace fs = std::filesystem;

    exports_t get_exports(const fs::path& lib_path) {
        int const fd = open(path::to_str(lib_path).c_str(), O_RDONLY);
        if(fd < 0) {
            LOG_ERROR("Failed to open library: %s", lib_path.c_str());
            return {};
        }

        struct stat st{};
        if(fstat(fd, &st) != 0) {
            LOG_ERROR("Failed to fstat library: %s", lib_path.c_str());
            close(fd);
            return {};
        }

        size_t const filesize = st.st_size;
        void* map = mmap(nullptr, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
        if(map == MAP_FAILED) {
            perror("mmap");
            LOG_ERROR("Failed to mmap library: %s", lib_path.c_str());
            close(fd);
            return {};
        }

        if(memcmp(map, ELFMAG, SELFMAG) != 0) {
            LOG_ERROR("Failed to identify library (not a valid ELF file): %s", lib_path.c_str());
            munmap(map, filesize);
            close(fd);
            return {};
        }

        exports_t exports = list_dynsym_exports(map);

        munmap(map, filesize);
        close(fd);

        return exports;
    }

    std::filesystem::path get_fs_path(void* const module_handle) {
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

    std::optional<void*> get_function_address(
        void* const module_handle,
        const char* function_name
    ) {
        if(auto* const address = dlsym(module_handle, function_name)) {
            return {address};
        }

        return {};
    }

    std::optional<section_t> get_section(void* lib_handle, const std::string& section_name) {
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

    std::optional<void*> load_library(const std::filesystem::path& library_path) {
        LOG_DEBUG("Loading library: '{}'", path::to_str(library_path));

        if(auto* library = dlopen(path::to_str(library_path).c_str(), RTLD_NOW | RTLD_GLOBAL)) {
            return {library};
        }

        return {};
    }

    void unload_library(void* library_handle) {
        dlclose(library_handle);
    }

    void* get_library_handle(const TCHAR* library_name) {
        return dlopen(library_name, RTLD_NOW | RTLD_GLOBAL);
    }
}
