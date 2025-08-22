#pragma once

#include <filesystem>

namespace koalabox::paths {
    namespace fs = std::filesystem;

    /**
     * @return std::path instance representing the directory containing this DLL.
     */
    fs::path get_self_path();

    fs::path get_config_path();

    fs::path get_cache_path();

    fs::path get_log_path();

    fs::path get_ca_key_path();

    fs::path get_ca_cert_path();

    fs::path get_cache_dir();

    fs::path get_user_dir();
}