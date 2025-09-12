#include <map>
#include <string>

#include <dlfcn.h>
#include <elf.h>
#include <libelf.h>
#include <link.h>

#include "koalabox/loader.hpp"

namespace koalabox::loader {
    std::map<std::string, std::string> get_export_map(void* const library, const bool /*undecorate*/) {
        // TODO: Validate implementation

        link_map const* map = nullptr;
        dlinfo(library, RTLD_DI_LINKMAP, &map);

        Elf64_Sym* symtab = nullptr;
        char const* strtab = nullptr;
        uint32_t symentries = 0;
        for(const auto* section = map->l_ld; section->d_tag != DT_NULL; ++section) {
            if(section->d_tag == DT_SYMTAB) {
                symtab = reinterpret_cast<Elf64_Sym*>(section->d_un.d_ptr);
            }
            if(section->d_tag == DT_STRTAB) {
                strtab = reinterpret_cast<char*>(section->d_un.d_ptr);
            }
            if(section->d_tag == DT_SYMENT) {
                symentries = section->d_un.d_val;
            }
        }

        if(symentries == 0) {
            return {};
        }

        const auto size = strtab - reinterpret_cast<char*>(symtab);

        std::map<std::string, std::string> result;

        for(int k = 0; k < size / symentries; ++k) {
            const auto* sym = &symtab[k];
            // If sym is function
            if(ELF64_ST_TYPE(symtab[k].st_info) == STT_FUNC) {
                //str is name of each symbol
                const auto* str = &strtab[sym->st_name];

                result[str] = str;
            }
        }

        return result;
    }

    std::string get_decorated_function(void* /*library*/, const std::string& function_name) {
        // No valid use case for this so far
        return function_name;
    }
}
