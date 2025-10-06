#include <elfio/elfio.hpp>

#include <koalabox/logger.hpp>
#include <koalabox/path.hpp>

#include "koalabox_tools/module.hpp"

namespace {
    using namespace ELFIO;

    //------------------------------------------------------------------------------
    bool load_internal(std::istream& stream, bool is_lazy = false) {
        std::array<char, EI_NIDENT> e_ident = {0};
        // Read ELF file signature
        stream.seekg(0);
        stream.read(e_ident.data(), sizeof(e_ident));

        // Is it ELF file?
        if(stream.gcount() != sizeof(e_ident) ||
           e_ident[EI_MAG0] != ELFMAG0 || e_ident[EI_MAG1] != ELFMAG1 ||
           e_ident[EI_MAG2] != ELFMAG2 || e_ident[EI_MAG3] != ELFMAG3) {
            LOG_ERROR("Unexpected identification index: {}", e_ident);
            return false;
        }

        if((e_ident[EI_CLASS] != ELFCLASS64) &&
           (e_ident[EI_CLASS] != ELFCLASS32)) {
            LOG_ERROR("Unexpected class: {}", e_ident);
            return false;
        }

        if((e_ident[EI_DATA] != ELFDATA2LSB) &&
           (e_ident[EI_DATA] != ELFDATA2MSB)) {
            LOG_ERROR("Unexpected encoding: {}", e_ident);
            return false;
        }

        endianess_convertor convertor;
        convertor.setup(e_ident[EI_DATA]);

        std::vector<std::unique_ptr<section>> sections_;
        address_translator addr_translator;
        std::unique_ptr<elf_header> header = nullptr;

        std::shared_ptr<compression_interface> compression = nullptr;

        const auto create_header = [&](
            unsigned char file_class,
            unsigned char encoding
        ) -> std::unique_ptr<elf_header> {
            std::unique_ptr<elf_header> new_header;

            if(file_class == ELFCLASS64) {
                new_header = std::unique_ptr<elf_header>(
                    new(std::nothrow) elf_header_impl<Elf64_Ehdr>(
                        &convertor, encoding, &addr_translator
                    )
                );
            } else if(file_class == ELFCLASS32) {
                new_header = std::unique_ptr<elf_header>(
                    new(std::nothrow) elf_header_impl<Elf32_Ehdr>(
                        &convertor, encoding, &addr_translator
                    )
                );
            } else {
                LOG_ERROR("Unexpected class in load_internal: {}", file_class);
                return nullptr;
            }

            return new_header;
        };

        const auto get_class = [&]() -> unsigned char {
            return header ? header->get_class() : 0;
        };

        //------------------------------------------------------------------------------
        const auto create_section = [&]() -> section* {
            if(auto file_class = get_class(); file_class == ELFCLASS64) {
                sections_.emplace_back(
                    new(std::nothrow) section_impl<Elf64_Shdr>(
                        &convertor, &addr_translator, compression
                    )
                );
            } else if(file_class == ELFCLASS32) {
                sections_.emplace_back(
                    new(std::nothrow) section_impl<Elf32_Shdr>(
                        &convertor, &addr_translator, compression
                    )
                );
            } else {
                sections_.pop_back();
                LOG_ERROR("Unexpected class in create_section: {}", file_class);

                return nullptr;
            }

            section* new_section = sections_.back().get();
            // new_section->set_index( static_cast<Elf_Half>( sections_.size() - 1 ) );

            return new_section;
        };

        //------------------------------------------------------------------------------
        const auto load_sections = [&](std::istream& stream, bool is_lazy) -> bool {
            unsigned char file_class = header->get_class();
            Elf_Half entry_size = header->get_section_entry_size();
            Elf_Half num = header->get_sections_num();
            Elf64_Off offset = header->get_sections_offset();

            if((num != 0 && file_class == ELFCLASS64 &&
                entry_size < sizeof(Elf64_Shdr)) ||
               (num != 0 && file_class == ELFCLASS32 &&
                entry_size < sizeof(Elf32_Shdr))) {
                LOG_ERROR("Unexpected class in load_sections: {}", file_class);
                return false;
            }

            for(Elf_Half i = 0; i < num; ++i) {
                section* sec = create_section();
                // sec->load( stream,
                //            static_cast<std::streamoff>( offset ) +
                //                static_cast<std::streampos>( i ) * entry_size,
                //            is_lazy );
                // To mark that the section is not permitted to reassign address
                // during layout calculation
                sec->set_address(sec->get_address());
            }

            // if ( Elf_Half shstrndx = get_section_name_str_index();
            //      SHN_UNDEF != shstrndx ) {
            //     string_section_accessor str_reader( sections[shstrndx] );
            //     for ( Elf_Half i = 0; i < num; ++i ) {
            //         Elf_Word section_offset = sections[i]->get_name_string_offset();
            //         const char* p = str_reader.get_string( section_offset );
            //         if ( p != nullptr ) {
            //             sections[i]->set_name( p );
            //         }
            //     }
            //      }

            return true;
        };

        header = create_header(e_ident[EI_CLASS], e_ident[EI_DATA]);
        if(nullptr == header) {
            LOG_ERROR("Failed to create header")
            return false;
        }
        if(!header->load(stream)) {
            LOG_ERROR("Failed to load header")
            return false;
        }

        load_sections( stream, is_lazy );
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

        elfio reader;
        if(!reader.load(module_path_str)) {
            LOG_ERROR("Failed to read ELF file: {}", module_path_str);

            // Try to debug the error:
            debug_load(module_path_str);

            return std::nullopt;
        }

        section* dynsym = reader.sections[".dynsym"];
        if(!dynsym) {
            LOG_ERROR("Failed to find dynamic symbol table section: {}", module_path_str);
            return std::nullopt;
        }

        exports_t results;

        const symbol_section_accessor symbols(reader, dynsym);
        for(auto i = 0U; i < symbols.get_symbols_num(); ++i) {
            std::string name;
            Elf64_Addr value;
            Elf_Xword size;
            unsigned char bind;
            unsigned char type;
            unsigned char other;
            Elf_Half section_index;

            symbols.get_symbol(i, name, value, size, bind, type, section_index, other);

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
