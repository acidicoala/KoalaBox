#include <dlfcn.h>
#include <linux/limits.h>

#include "koalabox/logger.hpp"
#include "koalabox/module.hpp"
#include "koalabox/path.hpp"

namespace koalabox::module {
    std::filesystem::path get_fs_path(const void* const module_handle) {
        char path[PATH_MAX]{};

        if(!module_handle) {
            // Get path to the current executable
            const auto len = readlink("/proc/self/exe", path, sizeof(path) - 1);
            if(len != -1) {
                path[len] = '\0';
                return path::from_str(path);
            }
        } else {
            // Get path to the shared object containing the given symbol
            Dl_info info;
            if(dladdr(module_handle, &info) && info.dli_fname) {
                return path::from_str(info.dli_fname);
            }
        }

        throw std::runtime_error("Failed to get path from module");
    }

    std::optional<void*> get_function_address(
        void* const module_handle,
        const char* function_name,
        const char* version
    ) {
        if(auto* const address = dlvsym(module_handle, function_name, version)) {
            return {address};
        }

        return {};
    }

    std::optional<void*> load_library(const std::filesystem::path& library_path) {
        LOG_DEBUG("Loading library: {}", path::to_str(library_path));

        if(auto* library = dlopen(path::to_str(library_path).c_str(), RTLD_NOW)) {
            return {library};
        }

        return {};
    }
}
