#include <polyhook2/Detour/NatDetour.hpp>
#include <polyhook2/PE/EatHook.hpp>
#include <polyhook2/Virtuals/VFuncSwapHook.hpp>

#include "koalabox/hook.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/str.hpp"
#include "koalabox/util.hpp"
#include "koalabox/win_util.hpp"

namespace {
    auto& get_address_map() {
        static std::map<std::string, uintptr_t> address_map;
        return address_map;
    }

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

    std::vector<PLH::IHook*> hooks;
    auto& address_map = get_address_map();
}

namespace koalabox::hook {
    namespace fs = std::filesystem;

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
        if(detour->hook() || trampoline == 0) {
            hooks.push_back(detour);
            address_map[function_name] = trampoline;
        } else {
            throw util::exception("Failed to hook function: {}", function_name);
        }
    }

    void detour_or_throw(
        const HMODULE& module_handle,
        const std::string& function_name,
        const uintptr_t callback_function
    ) {
        const auto address = reinterpret_cast<uintptr_t>(win_util::get_proc_address_or_throw(
            module_handle,
            function_name.c_str()
        ));

        detour_or_throw(address, function_name, callback_function);
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
            util::panic("Failed to hook function {} via Detour: {}", function_name, ex.what());
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
            util::panic("Failed to hook function {} via Detour: {}", function_name, ex.what());
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
            address_map[function_name] = orig_function_address;

            hooks.push_back(eat_hook);
        } else {
            delete eat_hook;

            throw util::exception("Failed to hook function: '{}'", function_name);
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
        if(instance) {
            // TODO: Use
            //   struct clazz {
            //       uintptr_t vtable[];
            //   };
            const auto* vtable = *(uintptr_t**) instance;

            if(const auto target_func = vtable[ordinal]; target_func == callback_function) {
                LOG_DEBUG(
                    "Function '{}' is already hooked. Skipping virtual function swap",
                    function_name
                );
                return;
            }
        }

        LOG_DEBUG(
            "Hooking '{}' at [[{}]+0x{:X}] via virtual function swap",
            function_name,
            instance,
            ordinal * sizeof(void*)
        );

        const PLH::VFuncMap redirect = {{ordinal, callback_function},};

        PLH::VFuncMap original_functions;
        auto* const swap = new PLH::VFuncSwapHook((char*) instance, redirect, &original_functions);

        if(swap->hook()) {
            address_map[function_name] = original_functions[ordinal];

            hooks.push_back(swap);
        } else {
            throw util::exception("Failed to hook function: {}", function_name);
        }
    }

    void swap_virtual_func(
        const void* instance,
        const std::string& function_name,
        const int ordinal,
        uintptr_t callback_function
    ) {
        try {
            swap_virtual_func_or_throw(instance, function_name, ordinal, callback_function);
        } catch(const std::exception& ex) {
            util::panic(
                "Failed to hook function {} via virtual function swap: {}",
                function_name,
                ex.what()
            );
        }
    }

    uintptr_t get_original_function(const HMODULE& library, const std::string& function_name) {
        return reinterpret_cast<uintptr_t>(win_util::get_proc_address(
            library,
            function_name.c_str()
        ));
    }

    uintptr_t get_original_address(const std::string& function_name) {
        if(not address_map.contains(function_name)) {
            util::panic("Address map does not contain function: {}", function_name);
        }

        return address_map.at(function_name);
    }

    void init(bool print_info) {
        LOG_DEBUG("Hooking initialization");

        // Initialize polyhook logger
        auto polyhook_logger = std::make_shared<PolyhookLogger>(print_info);
        PLH::Log::registerLogger(polyhook_logger);
    }

    bool is_hook_mode(const HMODULE& self_module, const std::string& orig_library_name) {
        const auto module_path = win_util::get_module_file_name(self_module);

        const auto self_name = fs::path(module_path).stem().string();

        return not str::eq(self_name, orig_library_name);
    }
}
