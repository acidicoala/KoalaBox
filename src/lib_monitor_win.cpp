#include <phnt_windows.h> // It should precede phnt.h

#define PHNT_VERSION PHNT_WINDOWS_10_RS2
#include <phnt.h>

#include <ranges>

#include "koalabox/lib.hpp"
#include "koalabox/lib_monitor.hpp"
#include "koalabox/logger.hpp"

#define CALL_NT_DLL(FUNC) \
    reinterpret_cast<decltype(&FUNC)>( \
        kb::lib::get_function_address(kb::lib::get_library_handle("ntdll"), #FUNC).value() \
    )

namespace {
    namespace kb = koalabox;
    using namespace kb::lib_monitor;

    PVOID cookie = nullptr;

    /**
     * @see PLDR_DLL_NOTIFICATION_FUNCTION
     */
    void __stdcall notification_listener(
        const ULONG NotificationReason,
        const PLDR_DLL_NOTIFICATION_DATA NotificationData,
        [[maybe_unused]] PVOID Context
    ) {
        // We're interested only in load events
        if(NotificationReason != LDR_DLL_NOTIFICATION_REASON_LOADED) {
            return;
        }

        details::on_library_loaded(NotificationData->Loaded.FullDllName->Buffer, NotificationData->Loaded.DllBase);
    }
}

namespace koalabox::lib_monitor {
    bool is_initialized() {
        return cookie != nullptr;
    }

    namespace details {
        void init() {
            const auto status_code = CALL_NT_DLL(LdrRegisterDllNotification)(
                0, // Flags
                notification_listener,
                nullptr, // context
                &cookie
            );

            if(status_code != STATUS_SUCCESS) {
                throw KB_RT_ERROR("Failed to register DLL notification. Status: {}", status_code);
            }
        }

        void shutdown() {
            const auto status_code = CALL_NT_DLL(LdrUnregisterDllNotification)(cookie);

            if(status_code != STATUS_SUCCESS) {
                LOG_ERROR("Failed to unregister DLL notification. Status: {}", status_code);
            }

            cookie = nullptr;
        }
    }
}
