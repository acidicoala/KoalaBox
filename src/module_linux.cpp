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

    exports_t list_dynsym_exports_64(const void* map) {
        const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(map);
        const Elf64_Shdr* shdrs = reinterpret_cast<const Elf64_Shdr*>((const char*) map + ehdr->e_shoff);
        const char* shstrtab = (const char*) map + shdrs[ehdr->e_shstrndx].sh_offset;

        const Elf64_Shdr* dynsym = nullptr;
        const Elf64_Shdr* dynstr = nullptr;
        for(int i = 0; i < ehdr->e_shnum; ++i) {
            const char* name = shstrtab + shdrs[i].sh_name;
            if(strcmp(name, ".dynsym") == 0) dynsym = &shdrs[i];
            else if(strcmp(name, ".dynstr") == 0) dynstr = &shdrs[i];
        }
        if(!dynsym || !dynstr) {
            LOG_ERROR(".dynsym or .dynstr not found");
            return {};
        }

        const Elf64_Sym* syms = (const Elf64_Sym*) ((const char*) map + dynsym->sh_offset);
        size_t nsyms = dynsym->sh_size / sizeof(Elf64_Sym);
        const char* strtab = (const char*) map + dynstr->sh_offset;

        exports_t exports;
        for(size_t i = 0; i < nsyms; ++i) {
            const Elf64_Sym& s = syms[i];
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

    exports_t list_dynsym_exports_32(const void* map) {
        const Elf32_Ehdr* ehdr = reinterpret_cast<const Elf32_Ehdr*>(map);
        const Elf32_Shdr* shdrs = reinterpret_cast<const Elf32_Shdr*>((const char*) map + ehdr->e_shoff);
        const char* shstrtab = (const char*) map + shdrs[ehdr->e_shstrndx].sh_offset;

        const Elf32_Shdr* dynsym = nullptr;
        const Elf32_Shdr* dynstr = nullptr;
        for(int i = 0; i < ehdr->e_shnum; ++i) {
            const char* name = shstrtab + shdrs[i].sh_name;
            if(strcmp(name, ".dynsym") == 0) dynsym = &shdrs[i];
            else if(strcmp(name, ".dynstr") == 0) dynstr = &shdrs[i];
        }
        if(!dynsym || !dynstr) {
            LOG_ERROR(".dynsym or .dynstr not found");
            return {};
        }

        const Elf32_Sym* syms = (const Elf32_Sym*) ((const char*) map + dynsym->sh_offset);
        size_t nsyms = dynsym->sh_size / sizeof(Elf32_Sym);
        const char* strtab = (const char*) map + dynstr->sh_offset;

        exports_t exports;
        for(size_t i = 0; i < nsyms; ++i) {
            const Elf32_Sym& s = syms[i];
            if(ELF32_ST_TYPE(s.st_info) == STT_FUNC &&
               (ELF32_ST_BIND(s.st_info) == STB_GLOBAL || ELF32_ST_BIND(s.st_info) == STB_WEAK) &&
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

        unsigned char* ident = static_cast<unsigned char*>(map);
        if(memcmp(ident, ELFMAG, SELFMAG) != 0) {
            LOG_ERROR("Failed to identify library (not a valid ELF file): %s", lib_path.c_str());
            munmap(map, filesize);
            close(fd);
            return {};
        }

        exports_t exports;
        if(ident[EI_CLASS] == ELFCLASS64) {
            exports = list_dynsym_exports_64(map);
        } else if(ident[EI_CLASS] == ELFCLASS32) {
            exports = list_dynsym_exports_32(map);
        } else {
            LOG_ERROR("Unknown ELF class: {}", ident[EI_CLASS]);
        }

        munmap(map, filesize);
        close(fd);

        return exports;
    }

    std::filesystem::path get_fs_path(const void* const module_handle) {
        char path[PATH_MAX]{};

        if(!module_handle) {
            // Get path to the current executable
            const auto len = readlink("/proc/self/exe", path, sizeof(path) - 1);
            if(len != -1) {
                path[len] = '\0';
                return path::from_str(path);
            }
        } else {
            // Get path to the shared object containing the given symbol
            Dl_info info{};
            const auto result = dladdr(module_handle, &info);
            // TODO: Doesn't actually work, always fails
            if(!result) {
                LOG_ERROR("dladdr result: {}", result);
            }
            if(dladdr(module_handle, &info) && info.dli_fname) {
                return path::from_str(info.dli_fname);
            }
        }

        throw std::runtime_error("Failed to get path from module");
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

    std::optional<section_t> get_section(const void* module_handle, const std::string& section_name) {
        const auto* base = reinterpret_cast<const uint8_t*>(module_handle);
        const auto* ehdr = reinterpret_cast<const Elf64_Ehdr*>(base);

        if(memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0)
            return {};

        const auto* sh_table = reinterpret_cast<const Elf64_Shdr*>(base + ehdr->e_shoff);
        const auto* shstr_sh = &sh_table[ehdr->e_shstrndx];
        auto shstrtab = reinterpret_cast<const char*>(base + shstr_sh->sh_offset);

        for(int i = 0; i < ehdr->e_shnum; ++i) {
            const char* name = shstrtab + sh_table[i].sh_name;
            if(section_name == name) {
                void* start = (void*) (base + sh_table[i].sh_addr);
                void* end = (void*) (base + sh_table[i].sh_addr + sh_table[i].sh_size);
                return section_t{start, end, static_cast<uint32_t>(sh_table[i].sh_size)};
            }
        }
        return {};
    }

    std::optional<void*> load_library(const std::filesystem::path& library_path) {
        LOG_DEBUG("Loading library: {}", path::to_str(library_path));

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
