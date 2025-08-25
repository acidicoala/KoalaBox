#include <polyhook2/Detour/NatDetour.hpp>
#include <polyhook2/PE/EatHook.hpp>
#include <polyhook2/Virtuals/VFuncSwapHook.hpp>

#include "koalabox/hook.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/path.hpp"
#include "koalabox/str.hpp"
#include "koalabox/util.hpp"
#include "koalabox/win.hpp"

namespace {
    class PolyhookLogger final : public PLH::Logger {
        bool print_info;

    public:
        explicit PolyhookLogger(const bool print_info) : print_info(print_info) {}

        void log_impl(const std::string& msg, const PLH::ErrorLevel level) const {
            if(level == PLH::ErrorLevel::INFO && print_info) {
                LOG_DEBUG("[PLH] {}", msg);
            } else if(level == PLH::ErrorLevel::WARN) {
                LOG_WARN("[PLH] {}", msg);
            } else if(level == PLH::ErrorLevel::SEV) {
                LOG_ERROR("[PLH] {}", msg);
            }
        }

        void log(const std::string& msg, const PLH::ErrorLevel level) override {
            log_impl(msg, level);
        }
    };

    struct hook_data {
        std::unique_ptr<PLH::IHook> hook;
        uintptr_t original_address = 0;
    };

    // Key is function name
    std::map<std::string, hook_data> hook_map;
}

namespace koalabox::hook {
    bool is_hooked(const std::string& function_name) {
        return hook_map.contains(function_name);
    }

    bool unhook(const std::string& function_name) {
        static std::mutex section;
        const std::lock_guard lock(section);

        if(not hook_map.contains(function_name)) {
            LOG_WARN("Cannot unhook '{}' since it is not found in the hook map");
            return false;
        }

        const auto success = hook_map[function_name].hook->unHook();
        hook_map.erase(function_name);

        return success;
    }

    void detour_or_throw(
        const uintptr_t address,
        const std::string& function_name,
        const uintptr_t callback_function
    ) {
        if(address == callback_function) {
            LOG_DEBUG("Function '{}' is already hooked. Skipping detour.", function_name);
        }

        LOG_DEBUG("Hooking '{}' at {} via Detour", function_name, reinterpret_cast<void*>(address));

        uint64_t trampoline = 0;

        auto* const detour = new PLH::NatDetour(address, callback_function, &trampoline);

#ifdef _WIN64
        detour->setDetourScheme(PLH::x64Detour::ALL);
#endif
        if(detour->hook()) {
            hook_map[function_name] = {
                .hook = std::unique_ptr<PLH::IHook>(detour),
                .original_address = static_cast<uintptr_t>(trampoline),
            };
        } else {
            delete detour;
            throw std::runtime_error(std::format("Failed to hook function: {}", function_name));
        }
    }

    void detour_or_throw(
        const HMODULE& module_handle,
        const std::string& function_name,
        const uintptr_t callback_function
    ) {
        const auto address = win::get_proc_address_or_throw(
            module_handle,
            function_name.c_str()
        );

        detour_or_throw(reinterpret_cast<uintptr_t>(address), function_name, callback_function);
    }

    void detour_or_warn(
        const uintptr_t address,
        const std::string& function_name,
        const uintptr_t callback_function
    ) {
        try {
            detour_or_throw(address, function_name, callback_function);
        } catch(const std::exception& ex) {
            LOG_WARN("Detour error: {}", ex.what());
        }
    }

    void detour_or_warn(
        const HMODULE& module_handle,
        const std::string& function_name,
        const uintptr_t callback_function
    ) {
        try {
            detour_or_throw(module_handle, function_name, callback_function);
        } catch(const std::exception& ex) {
            LOG_WARN("Detour error: {}", ex.what());
        }
    }

    void detour(
        const uintptr_t address,
        const std::string& function_name,
        const uintptr_t callback_function
    ) {
        try {
            detour_or_throw(address, function_name, callback_function);
        } catch(const std::exception& ex) {
            util::panic(
                std::format("Failed to hook function {} via Detour: {}", function_name, ex.what())
            );
        }
    }

    void detour(
        const HMODULE& module_handle,
        const std::string& function_name,
        const uintptr_t callback_function
    ) {
        try {
            detour_or_throw(module_handle, function_name, callback_function);
        } catch(const std::exception& ex) {
            util::panic(
                std::format("Failed to hook function {} via Detour: {}", function_name, ex.what())
            );
        }
    }

    void eat_hook_or_throw(
        const HMODULE& module_handle,
        const std::string& function_name,
        const uintptr_t callback_function
    ) {
        LOG_DEBUG("Hooking '{}' via EAT", function_name);

        uint64_t orig_function_address = 0;
        auto* const eat_hook = new PLH::EatHook(
            function_name,
            module_handle,
            callback_function,
            &orig_function_address
        );

        if(eat_hook->hook()) {
            hook_map[function_name] = {
                .hook = std::unique_ptr<PLH::IHook>(eat_hook),
                .original_address = static_cast<uintptr_t>(orig_function_address),
            };
        } else {
            delete eat_hook;

            throw std::runtime_error(std::format("Failed to hook function: '{}'", function_name));
        }
    }

    void eat_hook_or_warn(
        const HMODULE& module_handle,
        const std::string& function_name,
        uintptr_t callback_function
    ) {
        try {
            eat_hook_or_throw(module_handle, function_name, callback_function);
        } catch(const std::exception& ex) {
            LOG_WARN("Detour error: {}", ex.what());
        }
    }

    void swap_virtual_func_or_throw(
        const void* instance,
        const std::string& function_name,
        const int ordinal,
        uintptr_t callback_function
    ) {
        struct clazz {
            uintptr_t vtable[];
        };

        const auto* cls = static_cast<const clazz*>(instance);

        if(const auto target_func = cls->vtable[ordinal]; target_func == callback_function) {
            LOG_DEBUG(
                "Function '{}' is already hooked. Skipping virtual function swap",
                function_name
            );
            return;
        }

        LOG_DEBUG(
            "Hooking '{}' at [[{}]+0x{:X}] via virtual function swap",
            function_name,
            instance,
            ordinal * sizeof(void*)
        );

        const PLH::VFuncMap redirect = {{ordinal, callback_function}};

        PLH::VFuncMap original_functions;
        auto* const swap_hook = new PLH::VFuncSwapHook(
            static_cast<const char*>(instance),
            redirect,
            &original_functions
        );

        if(swap_hook->hook()) {
            hook_map[function_name] = {
                .hook = std::unique_ptr<PLH::IHook>(swap_hook),
                .original_address = static_cast<uintptr_t>(original_functions[ordinal]),
            };
        } else {
            throw std::runtime_error(std::format("Failed to hook function: {}", function_name));
        }
    }

    void swap_virtual_func(
        const void* instance,
        const std::string& function_name,
        const int ordinal,
        const uintptr_t callback_function
    ) {
        try {
            swap_virtual_func_or_throw(instance, function_name, ordinal, callback_function);
        } catch(const std::exception& ex) {
            util::panic(
                std::format(
                    "Failed to hook function {} via virtual function swap: {}",
                    function_name,
                    ex.what()
                )
            );
        }
    }

    uintptr_t get_module_function_address(
        const HMODULE& module_handle,
        const std::string& function_name
    ) {
        const auto address = win::get_proc_address(module_handle, function_name.c_str());
        return reinterpret_cast<uintptr_t>(address);
    }

    uintptr_t get_hooked_function_address(const std::string& function_name) {
        if(not hook_map.contains(function_name)) {
            util::panic(std::format("Hook map does not contain function: {}", function_name));
        }

        return hook_map[function_name].original_address;
    }

    void init(bool print_info) {
        LOG_DEBUG("Hooking initialization");

        // Initialize polyhook logger
        const auto polyhook_logger = std::make_shared<PolyhookLogger>(print_info);
        PLH::Log::registerLogger(polyhook_logger);
    }

    bool is_hook_mode(const HMODULE& self_module, const std::string& orig_library_name) {
        // E.g. C:/Library/api.dll
        const auto module_path = win::get_module_path(self_module);

        // E.g. api
        const auto self_name = path::to_str(module_path.stem());

        return not str::eq(self_name, orig_library_name);
    }
}
