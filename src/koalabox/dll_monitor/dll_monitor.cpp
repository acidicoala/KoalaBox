#include "dll_monitor.hpp"
#include "ntapi.hpp"

#include "../logger/logger.hpp"
#include "../util/util.hpp"

using namespace koalabox;

namespace dll_monitor {

    static PVOID cookie = nullptr;

    [[maybe_unused]]
    void init(
        const String& target_library_name,
        const std::function<void(HMODULE module)>& callback
    ) {
        HMODULE original_library = ::GetModuleHandle(util::to_wstring(target_library_name).c_str());

        if (original_library) {
            // First check if the target dll is already loaded

            logger::debug("Library {} is already loaded", target_library_name);

            callback(original_library);
        }

        // Then start listening for future DLLs

        logger::debug("Initializing DLL monitor");

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

            auto base_dll_name = util::to_string(NotificationData->Loaded.BaseDllName->Buffer);
            auto full_dll_name = util::to_string(NotificationData->Loaded.FullDllName->Buffer);

            logger::debug("DLL loaded - BaseDllName: '{}', FullDllName: '{}'",
                base_dll_name,
                full_dll_name
            );

            auto data = static_cast<CallbackData*>(context);

            if (util::strings_are_equal(data->target_library_name, base_dll_name)) {
                HMODULE loaded_module = win_util::get_module_handle(full_dll_name);

                data->callback(loaded_module);
            }

            // Do not delete data since it is re-used
            // delete data;
        };

        auto context = new CallbackData{
            .target_library_name=target_library_name + ".dll",
            .callback=callback,
        };

        static const auto LdrRegisterDllNotification = reinterpret_cast<_LdrRegisterDllNotification>(
            win_util::get_proc_address(
                win_util::get_module_handle("ntdll"),
                "LdrRegisterDllNotification"
            )
        );

        const auto status = LdrRegisterDllNotification(0, notification_listener, context,
            &cookie);
        if (status != STATUS_SUCCESS) {
            util::panic("dll_monitor::init",
                "Failed to register DLL listener. Status code: {}", status
            );
        }

        logger::debug("DLL monitor was successfully initialized");


    }

    [[maybe_unused]]
    void shutdown() {
        static const auto LdrUnregisterDllNotification = reinterpret_cast<_LdrUnregisterDllNotification>(
            win_util::get_proc_address(
                win_util::get_module_handle("ntdll"),
                "LdrUnregisterDllNotification"
            )
        );

        LdrUnregisterDllNotification(cookie);

        logger::debug("DLL monitor was successfully shut down");
    }
}
