#include <ntapi.hpp>

#include "koalabox/dll_monitor.hpp"

#include "koalabox/logger.hpp"
#include "koalabox/str.hpp"
#include "koalabox/util.hpp"
#include "koalabox/win.hpp"

namespace {
    PVOID cookie = nullptr;

    namespace fs = std::filesystem;
    namespace kb = koalabox;

#define CALL_NT_DLL(FUNC) \
    reinterpret_cast<_##FUNC>( \
        kb::win::get_proc_address(\
            kb::win::get_module_handle("ntdll"), \
            #FUNC \
        ) \
    )

    struct callback_context {
        std::set<std::string> remaining_modules;
        std::function<kb::dll_monitor::callback_multi_t> callback;
    };

    void shutdown_listener(const callback_context* context) {
        std::thread(
            // we have to copy the context pointer
            // since its reference won't be valid in the new thread.
            [context] {
                const auto status = CALL_NT_DLL(LdrUnregisterDllNotification)(cookie);
                cookie = nullptr;
                delete context;

                if(status != STATUS_SUCCESS) {
                    LOG_ERROR("Failed to unregister DLL listener. Status code: {}", status);
                }

                LOG_DEBUG("DLL monitor was successfully shut down");
            }
        ).detach();
    }
}

namespace koalabox::dll_monitor {
    void init_listener(
        const std::set<std::string>& target_library_names,
        const std::function<callback_multi_t>& callback
    ) {
        if(cookie) {
            LOG_ERROR("DLL monitor already initialized");
            return;
        }

        // First start listening for future DLLs

        LOG_DEBUG("Initializing DLL monitor");

        // Pre-process the notification
        const auto notification_listener = [](
            ULONG NotificationReason,
            // ReSharper disable once CppParameterMayBeConstPtrOrRef
            LDR_DLL_NOTIFICATION_DATA* NotificationData,
            void* raw_context
        ) {
            // Only interested in load events
            if(NotificationReason != LDR_DLL_NOTIFICATION_REASON_LOADED) {
                return;
            }

            static std::mutex section;
            const std::lock_guard lock(section);

            const auto full_dll_name = str::to_str(NotificationData->Loaded.FullDllName->Buffer);

            // It's better to use trace logging because this reveals filesystem paths
            LOG_TRACE("DLL loaded: '{}'", full_dll_name);

            const auto dll_name = fs::path(full_dll_name).stem().string();
            auto* context = static_cast<callback_context*>(raw_context);

            if(context->remaining_modules.contains(dll_name)) {
                LOG_DEBUG("Library '{}' has been loaded", dll_name);

                auto* const loaded_module = win::get_module_handle(full_dll_name.c_str());

                context->callback(loaded_module, dll_name);
                context->remaining_modules.erase(dll_name);

                if(context->remaining_modules.empty()) {
                    LOG_TRACE("Shutting down dll monitor");

                    shutdown_listener(context);
                }
            }
        };

        // Map library names to lowercase with extension
        std::set<std::string> filter;
        for(const auto& name : target_library_names) {
            filter.insert(str::to_lower(name));
        }

        const auto status = CALL_NT_DLL(LdrRegisterDllNotification)(
            0,
            notification_listener,
            new callback_context{
                .remaining_modules = filter,
                .callback = callback,
            },
            &cookie
        );

        if(status != STATUS_SUCCESS) {
            util::panic(std::format("Failed to register DLL listener. Status code: {}", status));
        }

        LOG_DEBUG("DLL monitor was successfully initialized");

        // Then check if the target dll is already loaded
        for(const auto& library_name : target_library_names) {
            auto* module_handle = GetModuleHandle(str::to_wstr(library_name).c_str());

            if(not module_handle) {
                continue;
            }

            LOG_DEBUG("Library is already loaded: '{}'", library_name);

            callback(module_handle, library_name);
        }
    }
}
