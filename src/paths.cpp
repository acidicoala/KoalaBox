#include <ShlObj.h>

#include "koalabox/globals.hpp"
#include "koalabox/paths.hpp"
#include "koalabox/util.hpp"
#include "koalabox/win.hpp"

namespace koalabox::paths {
    namespace {
        std::string get_file_name(const std::string& suffix) {
            return globals::get_project_name() + suffix;
        }
    }

    fs::path get_self_dir() {
        static auto* const self_handle = globals::get_self_handle();
        static const auto self_path = win::get_module_path(self_handle);
        return self_path.parent_path();
    }

    fs::path get_config_path() {
        static const auto path = get_self_dir() / get_file_name(".config.json");
        return path;
    }

    fs::path get_cache_path() {
        static const auto path = get_self_dir() / get_file_name(".cache.json");
        return path;
    }

    fs::path get_log_path() {
        static const auto path = get_self_dir() / get_file_name(".log.log");
        return path;
    }

    fs::path get_ca_key_path() {
        static const auto project_name = globals::get_project_name();
        static const auto path = get_self_dir() / (project_name + ".ca.key");
        return path;
    }

    fs::path get_ca_cert_path() {
        static const auto project_name = globals::get_project_name();
        static const auto path = get_self_dir() / (project_name + ".ca.crt");
        return path;
    }

    fs::path get_cache_dir() {
        static const auto path = get_self_dir() / "cache";
        create_directories(path);
        return path;
    }

    fs::path get_user_dir() {
        TCHAR buffer[MAX_PATH];
        if(SHGetSpecialFolderPath(nullptr, buffer, CSIDL_PROFILE, FALSE)) {
            // Path retrieved successfully, so print it out
            return {buffer};
        }

        throw std::runtime_error(
            std::format("Error retrieving user directory: {}", win::get_last_error())
        );
    }
}