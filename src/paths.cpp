#include "koalabox/paths.hpp"
#include "koalabox/globals.hpp"
#include "koalabox/module.hpp"

namespace {
    std::string get_file_name(const std::string& suffix) {
        return koalabox::globals::get_project_name() + suffix;
    }
}

namespace koalabox::paths {
    fs::path get_self_dir() {
        static auto* const self_handle = globals::get_self_handle();
        static const auto self_path = module::get_fs_path(self_handle);
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

    fs::path get_cache_dir() {
        static const auto path = get_self_dir() / "cache";
        create_directories(path);
        return path;
    }
}