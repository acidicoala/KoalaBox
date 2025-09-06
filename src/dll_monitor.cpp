#include <ntapi.hpp>

#include <ranges>
#include <set>

#include "koalabox/dll_monitor.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/str.hpp"
#include "koalabox/win.hpp"

#define CALL_NT_DLL(FUNC) \
    reinterpret_cast<_##FUNC>( \
        kb::win::get_proc_address(\
            kb::win::get_module_handle("ntdll"), \
            #FUNC \
        ) \
    )

namespace {
    namespace fs = std::filesystem;
    namespace kb = koalabox;
    using namespace kb::dll_monitor;

    std::map<const void*, callbacks_t> cookie_callbacks;

    void process_module(
        const callback_context_t* const callback_context,
        const std::string& dll_name,
        const HMODULE module_handle
    ) {
        const auto& [cookie] = *callback_context;
        auto& callbacks = cookie_callbacks.at(cookie);
        const auto& callback = callbacks.at(dll_name);
        const auto needs_erasing = callback(module_handle);

        if(not needs_erasing) { return; }

        callbacks.erase(dll_name);

        if(callbacks.empty()) {
            // we have to start a new thread for cases where we shut down right after initialization
            std::thread(
                // we have to copy the callback context pointer since its reference won't be valid in the new thread.
                [callback_context] { shutdown_listener(callback_context); }
            ).detach();
        }
    }

    void notification_listener(
        const ULONG NotificationReason,
        const LDR_DLL_NOTIFICATION_DATA* NotificationData,
        const void* context
    ) {
        // Only interested in load events
        if(NotificationReason != LDR_DLL_NOTIFICATION_REASON_LOADED) {
            return;
        }

        static std::mutex section;
        const std::lock_guard lock(section);

        const auto full_dll_name = kb::str::to_str(NotificationData->Loaded.FullDllName->Buffer);
        const auto dll_name = kb::str::to_lower(fs::path(full_dll_name).stem().string());

        LOG_DEBUG("DLL loaded: '{}'", dll_name);
        // Trace log paths for privacy
        LOG_TRACE("DLL path: '{}'", full_dll_name);

        const auto* const callback_context = static_cast<const callback_context_t*>(context);
        const auto& [cookie] = *callback_context;

        if(not cookie_callbacks.contains(cookie)) {
            LOG_WARN("Received a notification for a missing cookie: {}", cookie);
            return;
        }

        if(not cookie_callbacks.at(cookie).contains(dll_name)) { return; }

        auto* const module_handle = static_cast<HMODULE>(NotificationData->Loaded.DllBase);

        LOG_INFO("Target library '{}' has been loaded: {}", dll_name, static_cast<void*>(module_handle));

        process_module(callback_context, dll_name, module_handle);
    }

    void check_loaded_modules(const callback_context_t* const context) {
        const auto& [cookie] = *context;

        for(const auto& dll_name : cookie_callbacks.at(cookie) | std::views::keys) {
            auto* const module_handle = GetModuleHandle(kb::str::to_wstr(dll_name).c_str());

            if(not module_handle) { continue; }

            LOG_INFO("Library is already loaded: '{}'", dll_name);

            process_module(context, dll_name, module_handle);
        }
    }
}

namespace koalabox::dll_monitor {
    const callback_context_t* init_listener(const callbacks_t& callbacks) {
        LOG_DEBUG("Initializing DLL monitor with ");

        auto* context = new callback_context_t{};
        const auto status_code = CALL_NT_DLL(LdrRegisterDllNotification)(
            0, // Flags
            notification_listener,
            static_cast<void*>(context),
            &context->cookie
        );

        if(status_code != STATUS_SUCCESS) {
            throw std::runtime_error(std::format("Failed to register DLL listener. Status code: {}", status_code));
        }

        // Map library names to lowercase
        callbacks_t lowercase_callbacks;
        for(const auto& [dll_name, callback] : callbacks) {
            lowercase_callbacks[str::to_lower(dll_name)] = callback;
        }

        cookie_callbacks[context->cookie] = std::move(lowercase_callbacks);

        LOG_DEBUG("DLL monitor initialized: {}", context->cookie);

        check_loaded_modules(context);

        return context;
    }

    void shutdown_listener(const callback_context_t* const context) {
        if(not context) {
            throw std::runtime_error(std::format("{} -> Context is null", __func__));
        }

        const auto [cookie] = *context;
        delete context;

        if(not cookie_callbacks.contains(cookie)) {
            LOG_ERROR("{} -> Cookie not found: {}", __func__, cookie);
            return;
        }

        const auto status_code = CALL_NT_DLL(LdrUnregisterDllNotification)(cookie);

        if(status_code != STATUS_SUCCESS) {
            LOG_ERROR("DLL monitor unregister error. Status code: {}", status_code);
        }

        cookie_callbacks.erase(cookie);

        LOG_DEBUG("DLL monitor shut down: {}", cookie);
    }
}
