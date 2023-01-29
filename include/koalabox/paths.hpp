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

}
