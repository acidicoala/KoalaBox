#pragma once

#include <functional>
#include <map>
#include <string>

#include "koalabox/str.hpp"

/**
 * Cross-platform library monitor starts listening to library load events and
 * invokes corresponding callbacks when a target library is loaded.
 */
namespace koalabox::lib_monitor {
    /** DLL name without extension. */
    using dll_name_t = std::string;
    /** @returns boolean indicating if the callback should be removed.*/
    using callback_t = std::function<bool(void* module_handle)>;
    using callbacks_t = std::map<dll_name_t, callback_t, str::case_insensitive_compare>;

    bool is_initialized(); // platform-specific

    /** @throws runtime_error if there was an initialization error from kernel. */
    void init_listener(const callbacks_t& callbacks);
    void shutdown_listener();

    namespace details {
        callbacks_t& get_callbacks();
        void process_library(const std::string& lib_name, void* lib_handle);

        void init(); // platform-specific
        void shutdown(); // platform-specific
    }
}
