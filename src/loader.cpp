#include <regex>

#include "koalabox/loader.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/path.hpp"
#include "koalabox/util.hpp"
#include "koalabox/win.hpp"

namespace koalabox::loader {
    /**
     * Key is undecorated name, value is decorated name, if `undecorate` is set
     */
    std::map<std::string, std::string> get_export_map(const HMODULE& library, bool undecorate) {
        // Adapted from: https://github.com/mborne/dll2def/blob/master/dll2def.cpp

        auto exported_functions = std::map<std::string, std::string>();

        auto* dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(library);

        if(dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
            util::panic("e_magic  != IMAGE_DOS_SIGNATURE");
        }

        const auto* header = reinterpret_cast<PIMAGE_NT_HEADERS>(
            reinterpret_cast<BYTE*>(library) + dos_header->e_lfanew
        );

        if(header->Signature != IMAGE_NT_SIGNATURE) {
            util::panic("header->Signature != IMAGE_NT_SIGNATURE");
        }

        if(header->OptionalHeader.NumberOfRvaAndSizes <= 0) {
            util::panic("header->OptionalHeader.NumberOfRvaAndSizes <= 0");
        }

        const auto& data_dir = header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        const auto* exports =
            reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
                reinterpret_cast<BYTE*>(library) + data_dir.VirtualAddress);

        const PVOID names = reinterpret_cast<BYTE*>(library) + exports->AddressOfNames;

        // Iterate over the names and add them to the vector
        for(unsigned int i = 0; i < exports->NumberOfNames; i++) {
            std::string exported_name = reinterpret_cast<char*>(library) + static_cast<DWORD*>(
                                            names)[i];

            if(undecorate) {
                std::string undecorated_function = exported_name; // fallback value

                // Extract function name from decorated name
                static const std::regex expression(R"((?:^_)?(\w+)(?:@\d+$)?)");

                std::smatch matches;
                if(std::regex_match(exported_name, matches, expression)) {
                    if(matches.size() == 2) {
                        undecorated_function = matches[1];
                    } else {
                        LOG_WARN("Exported function regex size != 2: {}", exported_name);
                    }
                } else {
                    LOG_WARN("Exported function regex failed: {}", exported_name);
                }

                exported_functions.insert({undecorated_function, exported_name});
            } else {
                exported_functions.insert({exported_name, exported_name});
            }
        }

        return exported_functions;
    }

    std::string get_decorated_function(const HMODULE& library, const std::string& function_name) {
#ifdef _WIN64
        return function_name;
#else
        static std::map<HMODULE, std::map<std::string, std::string>> undecorated_function_maps;

        if(not
            undecorated_function_maps.contains(library)
        ) {
            undecorated_function_maps[library] = get_export_map(library, true);
        }

        return undecorated_function_maps[library][function_name];
#endif
    }

    HMODULE load_original_library(const fs::path& self_path, const std::string& orig_library_name) {
        const auto original_module_path = self_path / (orig_library_name + "_o.dll");

        auto* const original_module = win::load_library(original_module_path);

        LOG_INFO("Loaded original library from: '{}'", path::to_str(original_module_path));

        return original_module;
    }
}
