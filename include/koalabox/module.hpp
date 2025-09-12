#pragma once

#include <filesystem>

/// Cross-platform abstraction over dynamic libraries (.DLL / .SO files)
namespace koalabox::module {
    std::filesystem::path get_fs_path(const void* module_handle);
    std::optional<void*> get_function_address(
        void* module_handle,
        const char* function_name,
        const char* version = nullptr
    );
    std::optional<void*> load_library(const std::filesystem::path& library_path);
    void* load_library_or_throw(const std::filesystem::path& library_path);
}
