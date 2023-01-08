#include <koalabox/hook.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/util.hpp>
#include <koalabox/win_util.hpp>
#include <polyhook2/Virtuals/VFuncSwapHook.hpp>
#include <polyhook2/PE/EatHook.hpp>
#include <polyhook2/Detour/NatDetour.hpp>

namespace koalabox::hook {
    using namespace koalabox;

    class PolyhookLogger : public PLH::Logger {
        bool print_info;
    public:
        explicit PolyhookLogger(bool print_info) : print_info(print_info) {};

        void log_impl(const String& msg, PLH::ErrorLevel level) const {
            if (level == PLH::ErrorLevel::INFO && print_info) {
                LOG_TRACE("[Polyhook] {}", msg)
            } else if (level == PLH::ErrorLevel::WARN) {
                LOG_WARN("[Polyhook] {}", msg)
            } else if (level == PLH::ErrorLevel::SEV) {
                LOG_ERROR("[Polyhook] {}", msg)
            }
        }

        void log(std::string&& msg, PLH::ErrorLevel level) override {
            log_impl(msg, level);
        }

        void log(const String& msg, PLH::ErrorLevel level) override {
            log_impl(msg, level);
        }
    };

    Vector<PLH::IHook*> hooks; // NOLINT(cert-err58-cpp)

    KOALABOX_API(void) detour_or_throw(
        Map<String, uintptr_t>& address_map,
        const uintptr_t address,
        const String& function_name,
        const uintptr_t callback_function
    ) {
        LOG_DEBUG("Hooking '{}' at {} via Detour", function_name, (void*) address)

        uint64_t trampoline = 0;

        auto* const detour = new PLH::NatDetour(address, callback_function, &trampoline);

#ifdef _WIN64
        detour->setDetourScheme(static_cast<PLH::x64Detour::detour_scheme_t>(Detour::ALL));
#endif
        if (detour->hook() || trampoline == 0) {
            hooks.push_back(detour);
            address_map[function_name] = trampoline;
        } else {
            throw util::exception("Failed to hook function: {}", function_name);
        }
    }

    KOALABOX_API(void) detour_or_throw(
        Map<String, uintptr_t>& address_map,
        const HMODULE& module_handle,
        const String& function_name,
        const uintptr_t callback_function
    ) {
        const auto address = reinterpret_cast<uintptr_t>(
            win_util::get_proc_address_or_throw(module_handle, function_name.c_str())
        );

        detour_or_throw(address_map, address, function_name, callback_function);
    }

    KOALABOX_API(void) detour_or_warn(
        Map<String, uintptr_t>& address_map,
        const uintptr_t address,
        const String& function_name,
        const uintptr_t callback_function
    ) {
        try {
            hook::detour_or_throw(address_map, address, function_name, callback_function);
        } catch (const Exception& ex) {
            LOG_WARN("Detour error: {}", ex.what())
        }
    }

    KOALABOX_API(void) detour_or_warn(
        Map<String, uintptr_t>& address_map,
        const HMODULE& module_handle,
        const String& function_name,
        const uintptr_t callback_function
    ) {
        try {
            hook::detour_or_throw(address_map, module_handle, function_name, callback_function);
        } catch (const Exception& ex) {
            LOG_WARN("Detour error: {}", ex.what())
        }
    }

    KOALABOX_API(void) detour(
        Map<String, uintptr_t>& address_map,
        const uintptr_t address,
        const String& function_name,
        const uintptr_t callback_function
    ) {
        try {
            detour_or_throw(address_map, address, function_name, callback_function);
        } catch (const Exception& ex) {
            util::panic("Failed to hook function {} via Detour: {}", function_name, ex.what());
        }
    }

    KOALABOX_API(void) detour(
        Map<String, uintptr_t>& address_map,
        const HMODULE& module_handle,
        const String& function_name,
        const uintptr_t callback_function
    ) {
        try {
            detour_or_throw(address_map, module_handle, function_name, callback_function);
        } catch (const Exception& ex) {
            util::panic("Failed to hook function {} via Detour: {}", function_name, ex.what());
        }
    }

    KOALABOX_API(void) eat_hook_or_throw(
        Map<String, uintptr_t>& address_map,
        const HMODULE& module_handle,
        const String& function_name,
        uintptr_t callback_function
    ) {
        LOG_DEBUG("Hooking '{}' via EAT", function_name)

        uint64_t orig_function_address = 0;
        auto* const eat_hook = new PLH::EatHook(
            function_name,
            module_handle,
            callback_function,
            &orig_function_address
        );

        if (eat_hook->hook()) {
            address_map[function_name] = orig_function_address;

            hooks.push_back(eat_hook);
        } else {
            delete eat_hook;

            throw util::exception("Failed to hook function: '{}'", function_name);
        }
    }

    KOALABOX_API(void) eat_hook_or_warn(
        Map<String, uintptr_t>& address_map,
        const HMODULE& module_handle,
        const String& function_name,
        uintptr_t callback_function
    ) {
        try {
            hook::eat_hook_or_throw(address_map, module_handle, function_name, callback_function);
        } catch (const Exception& ex) {
            LOG_WARN("Detour error: {}", ex.what())
        }
    }

    KOALABOX_API(void) swap_virtual_func_or_throw(
        Map<String, uintptr_t>& address_map,
        const void* instance,
        const String& function_name,
        const int ordinal,
        uintptr_t callback_function
    ) {
        LOG_DEBUG(
            "Hooking '{}' at [[{}]+0x{:X}] via virtual function swap",
            function_name, instance, ordinal * sizeof(void*)
        )

        const PLH::VFuncMap redirect = {
            {ordinal, callback_function},
        };

        PLH::VFuncMap original_functions;
        auto* const swap = new PLH::VFuncSwapHook((char*) instance, redirect, &original_functions);

        if (swap->hook()) {
            address_map[function_name] = original_functions[ordinal];

            hooks.push_back(swap);
        } else {
            throw util::exception("Failed to hook function: {}", function_name);
        }
    }

    KOALABOX_API(void) swap_virtual_func(
        Map<String, uintptr_t>& address_map,
        const void* instance,
        const String& function_name,
        const int ordinal,
        uintptr_t callback_function
    ) {
        try {
            swap_virtual_func_or_throw(address_map, instance, function_name, ordinal, callback_function);
        } catch (const Exception& ex) {
            util::panic("Failed to hook function {} via virtual function swap: {}", function_name, ex.what());
        }
    }

    KOALABOX_API(uintptr_t) get_original_function(const HMODULE& library, const char* function_name) {
        return reinterpret_cast<uintptr_t>(
            win_util::get_proc_address(library, function_name)
        );
    }

    KOALABOX_API(uintptr_t) get_original_hooked_function(
        const Map<String,
            uintptr_t>& address_map,
        const char* function_name
    ) {
        if (not address_map.contains(function_name)) {
            util::panic("Address map does not contain function: {}", function_name);
        }

        return address_map.at(function_name);
    }

    KOALABOX_API(void) init(bool print_info) {
        LOG_DEBUG("Hooking initialization")

        // Initialize polyhook logger
        auto polyhook_logger = std::make_shared<PolyhookLogger>(print_info);
        PLH::Log::registerLogger(polyhook_logger);
    }

    KOALABOX_API(bool) is_hook_mode(const HMODULE& self_module, const String& orig_library_name) {
        const auto module_path = win_util::get_module_file_name(self_module);

        const auto self_name = Path(module_path).filename().string();

        return self_name < not_equals > orig_library_name + ".dll";
    }
}
