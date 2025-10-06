#include <elfio/elfio.hpp>

#include <koalabox/logger.hpp>
#include <koalabox/path.hpp>

#include "koalabox_tools/module.hpp"

namespace {
    //------------------------------------------------------------------------------
    bool load_internal(std::istream& stream, bool is_lazy = false) {
        std::array<char, ELFIO::EI_NIDENT> e_ident = {0};
        // Read ELF file signature
        stream.seekg(0);
        stream.read(e_ident.data(), sizeof(e_ident));

        // Is it ELF file?
        if(stream.gcount() != sizeof(e_ident) ||
           e_ident[ELFIO::EI_MAG0] != ELFIO::ELFMAG0 || e_ident[ELFIO::EI_MAG1] != ELFIO::ELFMAG1 ||
           e_ident[ELFIO::EI_MAG2] != ELFIO::ELFMAG2 || e_ident[ELFIO::EI_MAG3] != ELFIO::ELFMAG3) {
            LOG_ERROR("Unexpected identification index: {}", e_ident);
            return false;
        }

        if((e_ident[ELFIO::EI_CLASS] != ELFIO::ELFCLASS64) &&
           (e_ident[ELFIO::EI_CLASS] != ELFIO::ELFCLASS32)) {
            LOG_ERROR("Unexpected class: {}", e_ident);
            return false;
        }

        if((e_ident[ELFIO::EI_DATA] != ELFIO::ELFDATA2LSB) &&
           (e_ident[ELFIO::EI_DATA] != ELFIO::ELFDATA2MSB)) {
            LOG_ERROR("Unexpected encoding: {}", e_ident);
            return false;
        }

        // convertor.setup( e_ident[ELFIO::EI_DATA] );
        // header = create_header( e_ident[ELFIO::EI_CLASS], e_ident[ELFIO::EI_DATA] );
        // if ( nullptr == header ) {
        //     return false;
        // }
        // if ( !header->load( stream ) ) {
        //     return false;
        // }

        // load_sections( stream, is_lazy );
        // bool is_still_good = load_segments( stream, is_lazy );
        // return is_still_good;
        return true;
    }

    //------------------------------------------------------------------------------
    bool debug_load(const std::string& file_name, bool is_lazy = false) {
        std::unique_ptr<std::ifstream> pstream = std::make_unique<std::ifstream>();
        pstream->open(file_name.c_str(), std::ios::in | std::ios::binary);
        if(pstream == nullptr || !*pstream) {
            LOG_ERROR("Failed to open file stream")
            return false;
        }

        bool ret = load_internal(*pstream, is_lazy);

        if(!is_lazy) {
            pstream.reset();
        }

        return ret;
    }
}

namespace koalabox::tools::module {
    std::optional<exports_t> get_exports(const std::filesystem::path& module_path) {
        const auto module_path_str = path::to_str(module_path);

        ELFIO::elfio reader;
        if(!reader.load(module_path_str)) {
            LOG_ERROR("Failed to read ELF file: {}", module_path_str);

            // Try to debug the error:
            debug_load(module_path_str);

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

            if(
                section_index != ELFIO::SHN_UNDEF && // Exported, not just referenced
                (bind == ELFIO::STB_GLOBAL || bind == ELFIO::STB_WEAK) && // Global or weak binding
                (other & 0x3) == ELFIO::STV_DEFAULT && // Default visibility
                !name.empty()
            ) {
                results.insert(name);
            }
        }

        return results;
    }
}
