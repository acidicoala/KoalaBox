#include <koalabox/dll_monitor.hpp>
#include <koalabox/ntapi.hpp>
#include <koalabox/util.hpp>
#include <koalabox/win_util.hpp>

#include <Windows.h>

namespace koalabox::dll_monitor {

    PVOID cookie = nullptr;

    void init(
        const String& target_library_name,
        const std::function<void(const HMODULE& module_handle)>& callback
    ) {
        init(
            Vector<String>{target_library_name}, [=](const HMODULE& module_handle, const String&) {
                callback(module_handle);
            }
        );
    }

    void init(
        const Vector<String>& target_library_names,
        const std::function<void(const HMODULE& module_handle, const String& library_name)>& callback
    ) {
        if (cookie) {
            logger->error("[dll_monitor::init] Already initialized");
            return;
        }

        // First start listening for future DLLs

        logger->debug("Initializing DLL monitor");

        struct CallbackData {
            Vector<String> target_library_names;
            std::function<void(const HMODULE& module_handle, const String& library_name)> callback;
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

            auto* const data = static_cast<CallbackData*>(context);

            for (const auto& library_name: data->target_library_names) {
                if (util::strings_are_equal(library_name + ".dll", base_dll_name)) {
                    logger->debug("Library '{}' has been loaded", library_name);

                    auto* const loaded_module = win_util::get_module_handle(full_dll_name.c_str());

                    data->callback(loaded_module, library_name);
                }
            }

            // Do not delete data since it is re-used
            // delete data;
        };

        auto* const context = new CallbackData{
            .target_library_names = target_library_names,
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

        // Then check if the target dll is already loaded
        for (const auto& library_name: target_library_names) {
            try {
                auto* const original_library = win_util::get_module_handle_or_throw(library_name.c_str());

                logger->debug("Library is already loaded: '{}'", library_name);

                callback(original_library, library_name);
            } catch (const std::exception& ex) {}
        }
    }

    void shutdown() {
        static const auto LdrUnregisterDllNotification = reinterpret_cast<_LdrUnregisterDllNotification>(
            win_util::get_proc_address(win_util::get_module_handle("ntdll"), "LdrUnregisterDllNotification")
        );

        LdrUnregisterDllNotification(cookie);
        cookie = nullptr;

        logger->debug("DLL monitor was successfully shut down");
    }
}
