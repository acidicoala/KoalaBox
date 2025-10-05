#include <fstream>
#include <regex>

#include <nlohmann/json.hpp>
// ReSharper disable once CppUnusedIncludeDirective
#include <wil/stl.h> // This header is absolutely necessary for wil::GetModuleFileNameW to work
#include <wil/win32_helpers.h>

#include "koalabox/core.hpp"
#include "koalabox/lib.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/path.hpp"

namespace koalabox::lib {
    namespace kb = koalabox;

    std::filesystem::path get_fs_path(void* const lib_handle) {
        const auto wstr_path = wil::GetModuleFileNameW<std::wstring>(static_cast<HMODULE>(lib_handle));
        return path::from_wstr(wstr_path);
    }

    std::optional<void*> get_function_address(void* const lib_handle, const char* function_name) {
        if(auto* const address = GetProcAddress(static_cast<HMODULE>(lib_handle), function_name)) {
            return {address};
        }

        return {};
    }

    std::optional<section_t> get_section(void* const lib_handle, const std::string& section_name) {
        const auto* const dos_header = static_cast<PIMAGE_DOS_HEADER>(lib_handle);

        if(dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
            LOG_ERROR("Invalid DOS file: {}", kb::path::to_str(lib::get_fs_path(lib_handle)));
            return std::nullopt;
        }

        auto* const nt_header = reinterpret_cast<PIMAGE_NT_HEADERS>(
            static_cast<uint8_t*>(lib_handle) + dos_header->e_lfanew
        );

        if(nt_header->Signature != IMAGE_NT_SIGNATURE) {
            LOG_ERROR("Invalid NT signature: {}", kb::path::to_str(lib::get_fs_path(lib_handle)));
            return std::nullopt;
        }

        auto* section = IMAGE_FIRST_SECTION(nt_header);
        for(int i = 0; i < nt_header->FileHeader.NumberOfSections; i++, section++) {
            auto name = std::string(reinterpret_cast<char*>(section->Name), 8);
            name = name.substr(0, name.find('\0')); // strip null padding

            if(name == section_name) {
                auto* const section_start = static_cast<uint8_t*>(lib_handle) + section->PointerToRawData;
                return section_t{
                    .start_address = section_start,
                    .end_address = section_start + section->SizeOfRawData,
                    .size = section->SizeOfRawData,
                };
            }
        }

        LOG_ERROR("Section '{}' not found in: {}", section_name, kb::path::to_str(lib::get_fs_path(lib_handle)));
        return std::nullopt;
    }

    std::optional<void*> load(const std::filesystem::path& library_path) {
        const auto path_str = path::to_platform_str(library_path);
        return LoadLibraryW(path_str.c_str());
    }

    void unload(void* lib_handle) {
        FreeLibrary(static_cast<HMODULE>(lib_handle));
    }

    void* get_lib_handle(const std::string& lib_name) {
        return GetModuleHandleW(str::to_wstr(lib_name).c_str());
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
