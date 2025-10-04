#include <fstream>
#include <map>
#include <regex>

#include <nlohmann/json.hpp>
#include <wil/stl.h> // This header is absolutely necessary for wil::GetModuleFileNameW to work
#include <wil/win32_helpers.h>

#include "koalabox/core.hpp"
#include "koalabox/lib.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/path.hpp"

namespace {
    // TODO: Inline this function
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
    std::filesystem::path get_fs_path(void* const module_handle) {
        const auto wstr_path = wil::GetModuleFileNameW<std::wstring>(static_cast<HMODULE>(module_handle));
        return path::from_wstr(wstr_path);
    }

    std::optional<void*> get_function_address(void* const lib_handle, const char* function_name) {
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

    std::optional<exports_t> get_exports(void* const lib_handle) {
        // Adapted from: https://github.com/mborne/dll2def/blob/master/dll2def.cpp

        auto exported_functions = std::map<std::string, std::string>();

        const auto* dos_header = static_cast<const IMAGE_DOS_HEADER*>(lib_handle);

        if(dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
            LOG_ERROR("e_magic  != IMAGE_DOS_SIGNATURE");
            return std::nullopt;
        }

        const auto* const base = static_cast<const uint8_t*>(lib_handle);
        const auto* header = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos_header->e_lfanew);

        if(header->Signature != IMAGE_NT_SIGNATURE) {
            LOG_ERROR("header->Signature != IMAGE_NT_SIGNATURE");
            return std::nullopt;
        }

        if(header->OptionalHeader.NumberOfRvaAndSizes <= 0) {
            LOG_ERROR("header->OptionalHeader.NumberOfRvaAndSizes <= 0");
            return std::nullopt;
        }

        const auto& data_dir = header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        const auto* exports = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(base + data_dir.VirtualAddress);

        const auto* names = base + exports->AddressOfNames;

        // Iterate over the names and add them to the vector
        for(unsigned int i = 0; i < exports->NumberOfNames; i++) {
            const auto* name_ptr = base + reinterpret_cast<const DWORD*>(names)[i];
            std::string exported_name = reinterpret_cast<const char*>(name_ptr);

            exported_functions.insert({exported_name, exported_name});
        }

        return exported_functions;
    }

    exports_t get_exports_or_throw(void* lib_handle) {
        return get_exports(lib_handle) | throw_if_empty(
            std::format("Failed get library exports of {}", lib_handle)
        );
    }

    std::optional<Bitness> get_bitness(const std::filesystem::path& library_path) {
        std::ifstream file(library_path, std::ios::binary);
        if(!file.is_open()) {
            LOG_ERROR("Failed to read library: {}", path::to_str(library_path));
            return std::nullopt;
        }

        IMAGE_DOS_HEADER dos_header;
        file.read(reinterpret_cast<char*>(&dos_header), sizeof(dos_header));

        if(dos_header.e_magic != IMAGE_DOS_SIGNATURE) {
            LOG_ERROR("File is not a valid DOS library: {}", path::to_str(library_path));
            return std::nullopt;
        }

        // Seek to PE header
        file.seekg(dos_header.e_lfanew, std::ios::beg);

        // Read PE signature
        DWORD signature;
        file.read(reinterpret_cast<char*>(&signature), sizeof(signature));

        if(signature != IMAGE_NT_SIGNATURE) {
            LOG_ERROR("File is not a valid NT library: {}", path::to_str(library_path));
            return std::nullopt;
        }

        // Read file header
        IMAGE_FILE_HEADER fileHeader;
        file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));

        // Check machine type
        switch(fileHeader.Machine) {
        case IMAGE_FILE_MACHINE_I386: return Bitness::$32;
        case IMAGE_FILE_MACHINE_AMD64: return Bitness::$64;
        default:
            LOG_ERROR("Invalid library architecture: {}", fileHeader.Machine);
            return std::nullopt;
        }
    }
}
