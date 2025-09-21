#include <ranges>
#include <set>

#include "koalabox/lib_monitor.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/lib.hpp"
#include "koalabox/path.hpp"

namespace {
    using namespace koalabox::lib_monitor;

    void process_library(const std::string& lib_name, void* lib_handle) {
        auto& callbacks = details::get_callbacks();

        bool should_remove_callback;
        try {
            const auto& callback = callbacks.at(lib_name);
            should_remove_callback = callback(lib_handle);
        } catch(const std::exception& e) {
            LOG_ERROR("{} -> Exception raised during callback invocation: {}", lib_name, e.what());
            should_remove_callback = true;
        }

        if(!should_remove_callback) {
            return;
        }

        callbacks.erase(lib_name);

        if(callbacks.empty()) {
            // we have to start a new thread for cases where we shut down right after initialization
            std::thread(shutdown_listener).detach();
        }
    }

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

            process_library(lib_name, module_handle);
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

        void on_library_loaded(const TCHAR* filename, void* lib_handle) {
            if(!filename || !lib_handle) {
                return;
            }

            const auto lib_path = str::to_str(filename);
            const auto lib_name = path::to_str(std::filesystem::path(lib_path).stem());

#ifdef KB_DEBUG
            LOG_TRACE("DLL loaded: '{}' -> '{}", lib_name, lib_path);
#else
            LOG_DEBUG("DLL loaded: '{}'", lib_name);
#endif

            if(!get_callbacks().contains(lib_name)) {
                return;
            }

            LOG_INFO("Target library '{}' has been loaded: {}", lib_name, lib_handle);

            process_library(lib_name, lib_handle);
        }
    }
}
