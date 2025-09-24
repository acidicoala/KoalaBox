#include <map>
#include <regex>

#include <wil/stl.h> // This header is absolutely necessary for wil::GetModuleFileNameW to work
#include <wil/win32_helpers.h>
#include <nlohmann/json.hpp>

#include "koalabox/logger.hpp"
#include "koalabox/lib.hpp"
#include "koalabox/path.hpp"
#include "koalabox/str.hpp"
#include "koalabox/util.hpp"

namespace {
    PIMAGE_SECTION_HEADER get_pe_section_header_or_throw(
        const HMODULE& module_handle,
        const std::string& section_name
    ) {
        const auto* const dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);

        if(dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
            throw std::runtime_error("Invalid DOS file");
        }

        auto* const nt_header =
            reinterpret_cast<PIMAGE_NT_HEADERS>(
                reinterpret_cast<uint8_t*>(module_handle) + dos_header->e_lfanew);

        if(nt_header->Signature != IMAGE_NT_SIGNATURE) {
            throw std::runtime_error("Invalid NT signature");
        }

        auto* section = IMAGE_FIRST_SECTION(nt_header);
        for(int i = 0; i < nt_header->FileHeader.NumberOfSections; i++, section++) {
            auto name = std::string(reinterpret_cast<char*>(section->Name), 8);
            name = name.substr(0, name.find('\0')); // strip null padding

            if(name != section_name) {
                continue;
            }

            return section;
        }

        throw std::runtime_error(std::format("Section '{}' not found", section_name));
    }
}

namespace koalabox::lib {
    std::filesystem::path get_fs_path(const void* const module_handle) {
        const auto wstr_path = wil::GetModuleFileNameW<std::wstring>(static_cast<HMODULE>(module_handle));
        return path::from_wstr(wstr_path);
    }

    std::optional<void*> get_function_address(
        void* const lib_handle,
        const char* function_name
    ) {
        if(auto* const address = GetProcAddress(static_cast<HMODULE>(lib_handle), function_name)) {
            return {address};
        }

        return {};
    }

    std::optional<section_t> get_section(void* const lib_handle, const std::string& section_name) {
        try {
            const auto* const section = get_pe_section_header_or_throw(static_cast<HMODULE>(lib_handle), section_name);
            auto* const section_start = static_cast<uint8_t*>(lib_handle) + section->PointerToRawData;

            return section_t{
                .start_address = section_start,
                .end_address = section_start + section->SizeOfRawData,
                .size = section->SizeOfRawData,
            };
        } catch(std::exception& e) {
            LOG_ERROR("Exception while getting section '{}' from library '{}': {}", section_name, lib_handle, e.what());
            return {};
        }
    }

    std::optional<void*> load_library(const std::filesystem::path& library_path) {
        const auto path_str = path::to_platform_str(library_path);
        return LoadLibraryW(path_str.c_str());
    }

    void unload_library(void* library_handle) {
        FreeLibrary(static_cast<HMODULE>(library_handle));
    }

    void* get_library_handle(const std::string& library_name) {
        return GetModuleHandleW(str::to_wstr(library_name).c_str());
    }

    export_map_t get_export_map(const void* library, const bool undecorate) {
        // Adapted from: https://github.com/mborne/dll2def/blob/master/dll2def.cpp

        auto exported_functions = std::map<std::string, std::string>();

        const auto* dos_header = static_cast<const IMAGE_DOS_HEADER*>(library);

        if(dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
            util::panic("e_magic  != IMAGE_DOS_SIGNATURE");
        }

        const auto* const base = static_cast<const uint8_t*>(library);
        const auto* header = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos_header->e_lfanew);

        if(header->Signature != IMAGE_NT_SIGNATURE) {
            util::panic("header->Signature != IMAGE_NT_SIGNATURE");
        }

        if(header->OptionalHeader.NumberOfRvaAndSizes <= 0) {
            util::panic("header->OptionalHeader.NumberOfRvaAndSizes <= 0");
        }

        const auto& data_dir = header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        const auto* exports = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(base + data_dir.VirtualAddress);

        const auto* names = base + exports->AddressOfNames;

        // Iterate over the names and add them to the vector
        for(unsigned int i = 0; i < exports->NumberOfNames; i++) {
            const auto* name_ptr = base + reinterpret_cast<const DWORD*>(names)[i];
            std::string exported_name = reinterpret_cast<const char*>(name_ptr);

            if(undecorate) {
                std::string undecorated_function = exported_name; // fallback value

                // Extract function name from decorated name
                static const std::regex expression(R"((?:^_)?(\w+)(?:@\d+$)?)");

                if(std::smatch matches; std::regex_match(exported_name, matches, expression)) {
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

    std::string get_decorated_function([[maybe_unused]] const void* library, const std::string& function_name) {
#ifdef KB_64
        return function_name;
#else
        // So far, the only valid use case for has been a 32-bit EOSSDK DLL

        static std::map<const void*, std::map<std::string, std::string>> undecorated_function_maps;

        if(not undecorated_function_maps.contains(library)) {
            undecorated_function_maps[library] = get_export_map(library, true);
            LOG_DEBUG(
                "Populated export map of {} with {} entries",
                library, undecorated_function_maps.at(library).size()
            );
            LOG_TRACE("Export map:\n{}", nlohmann::json(undecorated_function_maps[library]).dump(2));
        }

        try {
            return undecorated_function_maps.at(library).at(function_name);
        } catch(const std::exception&) {
            LOG_WARN("Function '{}' not found in export map of {}", function_name, library);
            return function_name;
        }
#endif
    }
}
