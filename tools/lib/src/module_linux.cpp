// ELFIO must be included before linux headers to avoid name collisions
#include <elfio/elfio.hpp>

#include <elf.h>

#include <koalabox/logger.hpp>
#include <koalabox/path.hpp>

#include "koalabox_tools/module.hpp"

namespace koalabox::tools::module {
    std::optional<exports_t> get_exports(const std::filesystem::path& module_path) {
        const auto module_path_str = path::to_str(module_path);

        ELFIO::elfio reader;
        if(!reader.load(module_path_str)) {
            LOG_ERROR("Failed to read ELF file: {}", module_path_str);
            return std::nullopt;
        }

        ELFIO::section* dynsym = reader.sections[".dynsym"];
        if(!dynsym) {
            LOG_ERROR("Failed to find dynamic symbol table section: {}", module_path_str);
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
}