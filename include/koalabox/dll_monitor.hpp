#pragma once

#include <functional>
#include <map>
#include <string>

/**
 * DLL Monitor starts listening to DLL load events and invoked corresponding callbacks when target DLL is loaded.
 */
namespace koalabox::dll_monitor {
    /** DLL name without extension. */
    using dll_name_t = std::string;
    /** @returns boolean indicating if the callback should be removed.*/
    using callback_t = std::function<bool(HMODULE module_handle)>;
    using callbacks_t = std::map<dll_name_t, callback_t>;

    struct callback_context_t {
        void* cookie;
    };

    /** @throws runtime_error if there was an initialization error from kernel. */
    const callback_context_t* init_listener(const callbacks_t& callbacks);

    /** Unregisters DLL listener and frees the callback context. */
    void shutdown_listener(const callback_context_t* context);
}
