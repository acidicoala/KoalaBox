#include "koalabox/dll_monitor.hpp"

namespace koalabox::dll_monitor {
    const callback_context_t* init_listener(const callbacks_t& callbacks) {
        const auto* context = new callback_context_t{};

        // TODO: Implement

        return context;
    }

    void shutdown_listener(const callback_context_t* const context) {
        delete context;

        // TODO: Implement
    }
}
