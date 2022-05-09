#include <koalabox/loader.hpp>

#include <koalabox/util.hpp>
#include <koalabox/win_util.hpp>

#include <regex>

namespace koalabox::loader {

    Path get_module_dir(const HMODULE& handle) {
        const auto file_name = win_util::get_module_file_name(handle);

        const auto module_path = Path(util::to_wstring(file_name));

        return module_path.parent_path();
    }

    /**
     * Key is undecorated name, value is decorated name, if `undecorate` is set
     */
    Map<String, String> get_export_map(const HMODULE& library, bool undecorate) {
        // Adapted from: https://github.com/mborne/dll2def/blob/master/dll2def.cpp

        auto exported_functions = Map<String, String>();

        auto* dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(library);

        if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
            util::panic("e_magic  != IMAGE_DOS_SIGNATURE");
        }

        auto* header = reinterpret_cast<PIMAGE_NT_HEADERS>((BYTE*) library + dos_header->e_lfanew );

        if (header->Signature != IMAGE_NT_SIGNATURE) {
            util::panic("header->Signature != IMAGE_NT_SIGNATURE");
        }

        if (header->OptionalHeader.NumberOfRvaAndSizes <= 0) {
            util::panic("header->OptionalHeader.NumberOfRvaAndSizes <= 0");
        }

        auto virtual_address = header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        auto* exports = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>((BYTE*) library + virtual_address);

        PVOID names = (BYTE*) library + exports->AddressOfNames;

        // Iterate over the names and add them to the vector
        for (unsigned int i = 0; i < exports->NumberOfNames; i++) {
            String exported_name = (char*) library + ((DWORD*) names)[i];

            if (undecorate) {
                String undecorated_function = exported_name; // fallback value

                // Extract function name from decorated name
                static const std::regex expression(R"((?:^_)?(\w+)(?:@\d+$)?)");

                std::smatch matches;
                if (std::regex_match(exported_name, matches, expression)) {
                    if (matches.size() == 2) {
                        undecorated_function = matches[1];
                    } else {
                        logger->warn("Exported function regex size != 2: {}", exported_name);
                    }
                } else {
                    logger->warn("Exported function regex failed: {}", exported_name);
                }

                exported_functions.insert({undecorated_function, exported_name});
            } else {
                exported_functions.insert({exported_name, exported_name});
            }
        }

        return exported_functions;
    }

    [[maybe_unused]] String get_decorated_function(const HMODULE& library, const String& function_name) {
        if (util::is_x64()) {
            return function_name;
        }

        static Map<HMODULE, Map<String, String>> undecorated_function_maps;

        if (not undecorated_function_maps.contains(library)) {
            undecorated_function_maps[library] = get_export_map(library, true);
        }

        return undecorated_function_maps[library][function_name];

    }

    HMODULE load_original_library(const Path& self_directory, const String& orig_library_name) {
        const auto original_module_path = self_directory / (orig_library_name + "_o.dll");

        auto* const original_module = win_util::load_library(original_module_path);

        logger->info("📚 Loaded original library from: '{}'", original_module_path.string());

        return original_module;
    }

}
