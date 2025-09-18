#include <ntapi.hpp>

#include <ranges>

#include "koalabox/lib_monitor.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/lib.hpp"
#include "koalabox/str.hpp"

#define CALL_NT_DLL(FUNC) \
    reinterpret_cast<_##FUNC>( \
        kb::lib::get_function_address(kb::lib::get_library_handle("ntdll"), #FUNC).value() \
    )

namespace {
    namespace kb = koalabox;
    using namespace kb::lib_monitor;

    void* cookie = nullptr;

    void __stdcall notification_listener(
        const ULONG NotificationReason,
        const LDR_DLL_NOTIFICATION_DATA* NotificationData,
        [[maybe_unused]] const void* context
    ) {
        // We're interested only in load events
        if(NotificationReason != LDR_DLL_NOTIFICATION_REASON_LOADED) {
            return;
        }

        static std::mutex section;
        const std::lock_guard lock(section);

        const auto dll_path = kb::str::to_str(NotificationData->Loaded.FullDllName->Buffer);
        const auto dll_name = std::filesystem::path(dll_path).stem().string();

#ifdef KB_DEBUG
        LOG_TRACE("DLL loaded: '{}'", dll_path);
#else
        LOG_DEBUG("DLL loaded: '{}'", dll_name);
#endif

        if(!details::get_callbacks().contains(dll_name)) {
            return;
        }

        auto* const dll_handle = NotificationData->Loaded.DllBase;

        LOG_INFO("Target library '{}' has been loaded: {}", dll_name, static_cast<void*>(dll_handle));

        details::process_library(dll_name, dll_handle);
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
