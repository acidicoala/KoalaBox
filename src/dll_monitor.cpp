#include <ntapi.hpp>

#include "koalabox/dll_monitor.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/str.hpp"
#include "koalabox/util.hpp"
#include "koalabox/win_util.hpp"

namespace {

    PVOID cookie = nullptr;

}

namespace koalabox::dll_monitor {

    void init_listener( //
        const std::string& target_library_name,
        const std::function<void(const HMODULE& module_handle)>& callback
    ) {
        init_listener(
            std::vector{target_library_name},
            [=](const HMODULE& module_handle, const std::string&) { callback(module_handle); }
        );
    }

    void init_listener( //
        const std::vector<std::string>& target_library_names,
        const std::function<
            void(const HMODULE& module_handle, const std::string& library_name)
        >& callback
    ) {
        if (cookie) {
            LOG_ERROR("DLL monitor already initialized");
            return;
        }

        // First start listening for future DLLs

        LOG_DEBUG("Initializing DLL monitor");

        struct callback_data {
            std::vector<std::string> target_library_names;
            std::function<void(const HMODULE& module_handle, const std::string& library_name)>
                callback;
        };

        // Pre-process the notification
        const auto notification_listener = [](const ULONG NotificationReason,
                                              const PLDR_DLL_NOTIFICATION_DATA NotificationData,
                                              const PVOID context) {
            // Only interested in load events
            if (NotificationReason != LDR_DLL_NOTIFICATION_REASON_LOADED) {
                return;
            }

            const auto base_dll_name = str::to_str(NotificationData->Loaded.BaseDllName->Buffer);
            const auto full_dll_name = str::to_str(NotificationData->Loaded.FullDllName->Buffer);

            // It's better to use trace logging because this reveals filesystem paths
            LOG_TRACE("DLL loaded: '{}'", full_dll_name);

            for  ( //
                const auto* data = static_cast<callback_data*>(context);
                const auto& library_name : data->target_library_names
            ) {
                if (str::eq(library_name + ".dll", base_dll_name)) {
                    LOG_DEBUG("Library '{}' has been loaded", library_name);

                    auto* const loaded_module = win_util::get_module_handle(full_dll_name.c_str());

                    data->callback(loaded_module, library_name);
                }
            }

            // Do not delete data since it is re-used
            // delete data;
        };

        auto* const context = new callback_data{
            .target_library_names = target_library_names,
            .callback = callback,
        };

        static const auto LdrRegisterDllNotification =
            reinterpret_cast<_LdrRegisterDllNotification>(win_util::get_proc_address(
                win_util::get_module_handle("ntdll"), "LdrRegisterDllNotification"
            ));

        if (const auto status =
                LdrRegisterDllNotification(0, notification_listener, context, &cookie);
            status != STATUS_SUCCESS) {
            util::panic("Failed to register DLL listener. Status code: {}", status);
        }

        LOG_DEBUG("DLL monitor was successfully initialized");

        // Then check if the target dll is already loaded
        for (const auto& library_name : target_library_names) {
            auto* original_library = GetModuleHandle(str::to_wstr(library_name).c_str());

            if (not original_library) {
                continue;
            }

            LOG_DEBUG("Library is already loaded: '{}'", library_name);

            callback(original_library, library_name);
        }
    }

    void shutdown_listener() {
        std::thread([] {
            static const auto LdrUnregisterDllNotification =
                reinterpret_cast<_LdrUnregisterDllNotification>(win_util::get_proc_address(
                    win_util::get_module_handle("ntdll"), "LdrUnregisterDllNotification"
                ));

            const auto status = LdrUnregisterDllNotification(cookie);
            cookie = nullptr;

            if (status != STATUS_SUCCESS) {
                LOG_ERROR("Failed to unregister DLL listener. Status code: {}", status);
            }

            LOG_DEBUG("DLL monitor was successfully shut down");
        }).detach();
    }
}
