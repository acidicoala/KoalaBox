#include <ranges>

#include <dlfcn.h>

#include "koalabox.hpp"

namespace {
    namespace kb = koalabox;

    auto& get_global_callbacks() {
        static kb::dll_monitor::callbacks_t global_callbacks{};
        return global_callbacks;
    }

    void process_library(const std::string& lib_name, void* lib_handle) {
        auto& global_callbacks = get_global_callbacks();

        const auto callback = global_callbacks.at(lib_name);
        try {
            // ReSharper disable once CppTooWideScope
            const auto need_erasing = callback(lib_handle);

            if(need_erasing) {
                global_callbacks.erase(lib_name);
            }
        } catch(const std::exception& e) {
            LOG_ERROR("{} -> Exception raised during callback invocation: {}", lib_name, e.what());
        }

        if(global_callbacks.empty()) {
            kb::dll_monitor::shutdown_listener(nullptr);
        }
    }

    void check_loaded_modules() {
        for(const auto& dll_name : get_global_callbacks() | std::views::keys) {
            auto* const module_handle = kb::lib::get_library_handle(dll_name.c_str());

            if(not module_handle) { continue; }

            static std::mutex section;
            const std::lock_guard lock(section);

            LOG_INFO("Target library is already loaded: '{}'", dll_name);

            process_library(dll_name, module_handle);
        }
    }

    void on_library_loaded(const char* filename, void* lib_handle) {
        static std::set<std::string> loaded_modules;

        if(!filename || !lib_handle) {
            return;
        }

        const auto lib_path = kb::path::from_str(filename);
        const auto lib_name = kb::str::to_lower(kb::path::to_str(lib_path.stem()));

        if(!loaded_modules.contains(lib_name)) {
            loaded_modules.insert(lib_name);

            LOG_TRACE("Library path: '{}'", kb::path::to_str(kb::lib::get_fs_path(lib_handle)));
            LOG_DEBUG("Library loaded: '{}' @ {}", lib_name, lib_handle);
        }

        if(get_global_callbacks().contains(lib_name)) {
            static std::mutex section;
            const std::lock_guard lock(section);

            LOG_INFO("Target library '{}' has been loaded: {}", lib_name, lib_handle);

            process_library(lib_name, lib_handle);
        }
    }

    void* $dlopen(const char* filename, const int flag) {
        static const auto dlopen$ = KB_HOOK_GET_HOOKED_FN(dlopen);
        auto* const lib_handle = dlopen$(filename, flag);

        // LOG_TRACE("{}: file: {}, flag: {} -> {}", __func__, filename, flag, module_handle);
        on_library_loaded(filename, lib_handle);

        return lib_handle;
    }

    void* $dlmopen(const Lmid_t namespace_id, const char* filename, const int flag) {
        static const auto dlmopen$ = KB_HOOK_GET_HOOKED_FN(dlmopen);
        auto* const lib_handle = dlmopen$(namespace_id, filename, flag);

        // LOG_TRACE("{}: nid: {}, file: {}, flag: {} -> {}", __func__, namespace_id, filename, flag, module_handle);
        on_library_loaded(filename, lib_handle);

        return lib_handle;
    }
}

namespace koalabox::dll_monitor {
    const callback_context_t* init_listener(const callbacks_t& callbacks) {
        LOG_DEBUG("Initializing DLL monitor");

        auto& global_callbacks = get_global_callbacks();
        global_callbacks.clear();

        std::stringstream joined_libs;
        for(const auto& [dll_name, callback] : callbacks) {
            const auto lower_name = str::to_lower(dll_name);
            global_callbacks[lower_name] = callback;
            joined_libs << std::format("'{}', ", lower_name);
        }
        LOG_DEBUG("Listening for {} libraries: {}", global_callbacks.size(), joined_libs.str());

#define HOOK(FUNC) hook::detour(reinterpret_cast<void*>(FUNC), #FUNC, reinterpret_cast<void*>($##FUNC))

        HOOK(dlopen);
        HOOK(dlmopen);

        LOG_DEBUG("DLL monitor initialized");

        check_loaded_modules();

        // A dummy context
        return new callback_context_t{};
    }

    void shutdown_listener(const callback_context_t* const context) {
        delete context;

        const auto dlopen_result = hook::unhook("dlopen");
        const auto dlmopen_result = hook::unhook("dlmopen");
        const auto result = dlopen_result && dlmopen_result;

        LOG_DEBUG("DLL monitor shut down: {}", result);
    }
}
