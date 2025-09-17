// ReSharper disable once CppUnusedIncludeDirective
#include <wil/stl.h> // This header is absolutely necessary for wil::GetModuleFileNameW to work
#include <wil/win32_helpers.h>

#include "koalabox/module.hpp"
#include "koalabox/path.hpp"

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

namespace koalabox::module {
    std::filesystem::path get_fs_path(void* const module_handle) {
        const auto wstr_path = wil::GetModuleFileNameW<std::wstring>(static_cast<HMODULE>(module_handle));
        return path::from_wstr(wstr_path);
    }

    std::optional<void*> get_function_address(
        void* const module_handle,
        const char* function_name
    ) {
        if(auto* const address = GetProcAddress(static_cast<HMODULE>(module_handle), function_name)) {
            return {address};
        }

        return {};
    }

    std::optional<section_t> get_section(void* const lib_handle, const std::string& section_name) {
        const auto section = get_pe_section_header_or_throw(static_cast<HMODULE>(lib_handle), section_name);
        const auto section_start = static_cast<uint8_t*>(lib_handle) + section->PointerToRawData;

        return section_t{
            .start_address = section_start,
            .end_address = section_start + section->SizeOfRawData,
            .size = section->SizeOfRawData,
        };
    }

    std::optional<void*> load_library(const std::filesystem::path& library_path) {
        const auto path_str = path::to_kb_str(library_path);
        return LoadLibrary(path_str.c_str());
    }

    void unload_library(void* library_handle) {
        FreeLibrary(static_cast<HMODULE>(library_handle));
    }

    void* get_library_handle(const TCHAR* library_name) {
        return GetModuleHandle(library_name);
    }
}
