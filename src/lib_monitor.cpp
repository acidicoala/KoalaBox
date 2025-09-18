#include <ranges>

#include "koalabox/lib_monitor.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/lib.hpp"

namespace {
    using namespace koalabox::lib_monitor;

    void check_loaded_modules() {
        // The map might change during iteration, so we need to iterate over a copy of keys
        auto keys = std::views::keys(details::get_callbacks());
        const std::set lib_names(keys.begin(), keys.end());

        for(const auto& lib_name : lib_names) {
            auto* const module_handle = koalabox::lib::get_library_handle(lib_name);

            if(not module_handle) {
                continue;
            }

            LOG_INFO("Library is already loaded: '{}'", lib_name);

            details::process_library(lib_name, module_handle);
        }
    }
}

namespace koalabox::lib_monitor {
    void init_listener(const callbacks_t& callbacks) {
        if(is_initialized()) {
            LOG_WARN("Library monitor is already initialized.");
            return;
        }
        LOG_DEBUG("Initializing library monitor...");

        details::get_callbacks() = callbacks;
        details::init();

        LOG_DEBUG("Library monitor initialized.");

        check_loaded_modules();
    }

    void shutdown_listener() {
        if(!is_initialized()) {
            LOG_WARN("Library monitor is already shut down.");
            return;
        }

        LOG_DEBUG("Shutting down library monitor...");

        details::shutdown();
        details::get_callbacks().clear();

        LOG_DEBUG("Library monitor shut down.");
    }

    namespace details {
        callbacks_t& get_callbacks() {
            static callbacks_t callbacks;
            return callbacks;
        }

        void process_library(const std::string& lib_name, void* lib_handle) {
            bool should_remove_callback;
            try {
                const auto& callback = get_callbacks().at(lib_name);
                should_remove_callback = callback(lib_handle);
            } catch(const std::exception& e) {
                LOG_ERROR("{} -> Exception raised during callback invocation: {}", lib_name, e.what());
                should_remove_callback = true;
            }

            if(!should_remove_callback) {
                return;
            }

            get_callbacks().erase(lib_name);

            if(get_callbacks().empty()) {
                // we have to start a new thread for cases where we shut down right after initialization
                std::thread(shutdown_listener).detach();
            }
        }
    }
}
