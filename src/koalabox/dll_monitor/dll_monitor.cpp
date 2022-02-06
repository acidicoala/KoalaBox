#include "dll_monitor.hpp"
#include "ntapi.hpp"

#include "../logger/logger.hpp"
#include "../util/util.hpp"

using namespace koalabox;

_LdrRegisterDllNotification LdrRegisterDllNotification = nullptr;
_LdrUnregisterDllNotification LdrUnregisterDllNotification = nullptr;

void init_nt_functions() {
    const HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (hNtDll == nullptr) {
        util::panic(__func__, "Failed to get a handle for ntdll.dll module");
    }
    LdrRegisterDllNotification = reinterpret_cast<_LdrRegisterDllNotification>(
        GetProcAddress(hNtDll, "LdrRegisterDllNotification")
    );
    LdrUnregisterDllNotification = reinterpret_cast<_LdrUnregisterDllNotification>(
        GetProcAddress(hNtDll, "LdrUnregisterDllNotification")
    );

    if (!LdrRegisterDllNotification || !LdrUnregisterDllNotification) {
        util::panic(__func__, "Some ntdll procedures were not found");
    }
}

[[maybe_unused]]
HMODULE dll_monitor::init(
    const String& target_dll,
    const std::function<void(HMODULE module)>& callback
) {
    // First check if the target dll is already loaded
    HMODULE original_module = ::GetModuleHandleA(target_dll.c_str());

    if (original_module == nullptr) {
        // Otherwise, initialize the monitor

        logger::debug("Initializing DLL monitor");

        init_nt_functions();

        struct CallbackData {
            String target_dll;
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

            logger::debug("DLL monitor notification listener callback");
            logger::debug("buffer: {}", (void*) NotificationData->Loaded.BaseDllName->Buffer);

            auto dll_name = util::to_string(
                WideString(NotificationData->Loaded.BaseDllName->Buffer)
            );

            logger::debug("DLL loaded: {}", dll_name);

            auto data = static_cast<CallbackData*>(context);

            if (util::strings_are_equal(data->target_dll, dll_name)) {
                HMODULE loaded_module = win_util::get_module_handle(data->target_dll);

                data->callback(loaded_module);
            }

            delete data;
        };

        static PVOID cookie = nullptr;

        auto context = new CallbackData{
            .target_dll=target_dll,
            .callback=callback,
        };
        const auto status = LdrRegisterDllNotification(0, notification_listener, context, &cookie);
        if (status != STATUS_SUCCESS) {
            util::panic("dll_monitor::init",
                "Failed to register DLL listener. Status code: {}", status
            );
        }

        logger::debug("DLL monitor was successfully initialized");

    }

    return original_module;
}
