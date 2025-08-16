#pragma once

#include <koalabox/core.hpp>

namespace koalabox::paths {

    /**
     * @return An std::path instance representing the directory containing this DLL
     */
    KOALABOX_API(Path) get_self_path();

    KOALABOX_API(Path) get_config_path();

    KOALABOX_API(Path) get_cache_path();

    KOALABOX_API(Path) get_log_path();

    KOALABOX_API(Path) get_ca_key_path();

    KOALABOX_API(Path) get_ca_cert_path();

    KOALABOX_API(Path) get_cache_dir();

    KOALABOX_API(Path) get_user_dir();

}

#define TEST_TEST ABC123
