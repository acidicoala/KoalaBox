#include "dll_monitor.hpp"
#include "ntapi.hpp"

#include "koalabox/win_util/win_util.hpp"
#include "koalabox/util/util.hpp"

namespace koalabox::dll_monitor {

    PVOID cookie = nullptr;

    void init(
        const String& target_library_name,
        const std::function<void(const HMODULE& module)>& callback
    ) {
        // First check if the target dll is already loaded
        try {
            const auto original_library = win_util::get_module_handle_or_throw(target_library_name.c_str());

            logger->debug("Library is already loaded: '{}'", target_library_name);

            callback(original_library);
        } catch (const std::exception& ex) {}

        // Then start listening for future DLLs

        logger->debug("Initializing DLL monitor");

        struct CallbackData {
            String target_library_name;
            std::function<void(HMODULE module)> callback;
        };

        // Pre-process the notification
        const auto notification_listener = [](
            ULONG NotificationReason,
            PLDR_DLL_NOTIFICATION_DATA NotificationData,
            PVOID context
        ) {
            // Only interested in load events
            if (NotificationReason != LDR_DLL_NOTIFICATION_REASON_LOADED) {
                return;
            }

            const auto base_dll_name = util::to_string(NotificationData->Loaded.BaseDllName->Buffer);
            const auto full_dll_name = util::to_string(NotificationData->Loaded.FullDllName->Buffer);

            const auto data = static_cast<CallbackData*>(context);

            if (util::strings_are_equal(data->target_library_name, base_dll_name)) {
                logger->debug("Library {} has been loaded", data->target_library_name);

                HMODULE loaded_module = win_util::get_module_handle(full_dll_name.c_str());

                data->callback(loaded_module);
            }

            // Do not delete data since it is re-used
            // delete data;
        };

        const auto context = new CallbackData{
            .target_library_name = target_library_name + ".dll",
            .callback = callback,
        };

        static const auto LdrRegisterDllNotification = reinterpret_cast<_LdrRegisterDllNotification>(
            win_util::get_proc_address(win_util::get_module_handle("ntdll"), "LdrRegisterDllNotification")
        );

        const auto status = LdrRegisterDllNotification(0, notification_listener, context, &cookie);

        if (status != STATUS_SUCCESS) {
            util::panic("Failed to register DLL listener. Status code: {}", status);
        }

        logger->debug("DLL monitor was successfully initialized");
    }

    void shutdown() {
        static const auto LdrUnregisterDllNotification = reinterpret_cast<_LdrUnregisterDllNotification>(
            win_util::get_proc_address(win_util::get_module_handle("ntdll"), "LdrUnregisterDllNotification")
        );

        LdrUnregisterDllNotification(cookie);

        logger->debug("DLL monitor was successfully shut down");
    }
}
