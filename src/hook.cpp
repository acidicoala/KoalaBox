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
            const auto trimmed_message_view = std::string_view(
                msg.data(),
                msg.empty() ? 0 : msg.size() - 1
            );

            if(level == PLH::ErrorLevel::INFO && print_info) {
                LOG_DEBUG("[PLH] {}", trimmed_message_view);
            } else if(level == PLH::ErrorLevel::WARN) {
                LOG_WARN("[PLH] {}", trimmed_message_view);
            } else if(level == PLH::ErrorLevel::SEV) {
                LOG_ERROR("[PLH] {}", trimmed_message_view);
            }
        }

        void log(const std::string& msg, const PLH::ErrorLevel level) override {
            log_impl(msg, level);
        }
    };

    struct hook_data {
        std::unique_ptr<PLH::IHook> hook;
        void* orig_func_ptr = nullptr;
    };

    // Key is function name
    using function_to_hook_data_map = std::map<std::string, hook_data>;

    // Key is class name
    std::map<const void*, function_to_hook_data_map> class_map;

    function_to_hook_data_map hook_map;
}

namespace koalabox::hook {
    bool is_hooked(const std::string& function_name) {
        return hook_map.contains(function_name);
    }

    bool is_vt_hooked(const void* class_ptr, const std::string& function_name) {
        return class_map.contains(class_ptr) &&
               class_map.at(class_ptr).contains(function_name);
    }

    bool unhook(const std::string& function_name) {
        static std::mutex section;
        const std::lock_guard lock(section);

        if(not hook_map.contains(function_name)) {
            LOG_ERROR("Cannot unhook '{}'. Function name not found", function_name);
            return false;
        }

        const auto success = hook_map.at(function_name).hook->unHook();
        hook_map.erase(function_name);

        return success;
    }

    bool unhook_vt(const void* class_ptr, const std::string& function_name) {
        static std::mutex section;
        const std::lock_guard lock(section);

        if(not class_map.contains(class_ptr)) {
            LOG_ERROR("Cannot unhook '{}'. Class pointer not found: {}", function_name, class_ptr)
            return false;
        }

        auto& function_map = class_map.at(class_ptr);

        if(not function_map.contains(function_name)) {
            LOG_ERROR("Cannot unhook '{}'. Function name not found", function_name);
            return false;
        }

        const auto success = function_map.at(function_name).hook->unHook();
        function_map.erase(function_name);

        return success;
    }

    void detour_or_throw(
        const void* address,
        const std::string& function_name,
        const void* callback_function
    ) {
        if(address == callback_function) {
            LOG_DEBUG("Function '{}' is already hooked. Skipping detour.", function_name);
        }

        LOG_DEBUG("Hooking '{}' at {} via Detour", function_name, address);

        uint64_t trampoline = 0;

        auto* const detour = new PLH::NatDetour(
            reinterpret_cast<uint64_t>(address),
            reinterpret_cast<uint64_t>(callback_function),
            &trampoline
        );

#ifdef _WIN64
        detour->setDetourScheme(PLH::x64Detour::ALL);
#endif
        if(detour->hook()) {
            hook_map[function_name] = {
                .hook = std::unique_ptr<PLH::IHook>(detour),
                .orig_func_ptr = reinterpret_cast<void*>(trampoline),
            };
        } else {
            delete detour;
            throw std::runtime_error(std::format("Failed to hook function: {}", function_name));
        }
    }

    void detour_or_throw(
        const HMODULE& module_handle,
        const std::string& function_name,
        const void* callback_function
    ) {
        const auto address = win::get_proc_address_or_throw(
            module_handle,
            function_name.c_str()
        );

        detour_or_throw(reinterpret_cast<void*>(address), function_name, callback_function);
    }

    void detour_or_warn(
        const void* address,
        const std::string& function_name,
        const void* callback_function
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
        const void* callback_function
    ) {
        try {
            detour_or_throw(module_handle, function_name, callback_function);
        } catch(const std::exception& ex) {
            LOG_WARN("Detour error: {}", ex.what());
        }
    }

    void detour(
        const void* address,
        const std::string& function_name,
        const void* callback_function
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
        const void* callback_function
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
        const void* callback_function
    ) {
        LOG_DEBUG("Hooking '{}' via EAT", function_name);

        uint64_t orig_function_address = 0;

        auto* const eat_hook = new PLH::EatHook(
            function_name,
            module_handle,
            reinterpret_cast<uint64_t>(callback_function),
            &orig_function_address
        );

        if(eat_hook->hook()) {
            hook_map[function_name] = {
                .hook = std::unique_ptr<PLH::IHook>(eat_hook),
                .orig_func_ptr = reinterpret_cast<void*>(orig_function_address),
            };
        } else {
            delete eat_hook;

            throw std::runtime_error(std::format("Failed to hook function: '{}'", function_name));
        }
    }

    void eat_hook_or_warn(
        const HMODULE& module_handle,
        const std::string& function_name,
        const void* callback_function
    ) {
        try {
            eat_hook_or_throw(module_handle, function_name, callback_function);
        } catch(const std::exception& ex) {
            LOG_WARN("Detour error: {}", ex.what());
        }
    }

    void swap_virtual_func_or_throw(
        const void* class_ptr,
        const std::string& function_name,
        const uint16_t ordinal,
        const void* callback_function
    ) {
        struct clazz {
            void* vtable[];
        };

        const auto* cls = static_cast<const clazz*>(class_ptr);

        const auto func_data = std::format(
            "'{}' @ [[{}]+0x{:X}]",
            function_name,
            class_ptr,
            ordinal * sizeof(void*)
        );

        if(const auto* target_func = cls->vtable[ordinal]; target_func == callback_function) {
            LOG_DEBUG("Function {} is already hooked. Skipping virtual function swap", func_data);
            return;
        }

        LOG_DEBUG("Hooking {} via virtual function swap", func_data);

        const PLH::VFuncMap redirect = {{ordinal, reinterpret_cast<uint64_t>(callback_function)}};

        PLH::VFuncMap original_functions;

        auto* const swap_hook = new PLH::VFuncSwapHook(
            static_cast<const char*>(class_ptr),
            redirect,
            &original_functions
        );

        if(swap_hook->hook()) {
            class_map[class_ptr][function_name] = {
                .hook = std::unique_ptr<PLH::IHook>(swap_hook),
                .orig_func_ptr = reinterpret_cast<void*>(original_functions[ordinal]),
            };
        } else {
            throw std::runtime_error(std::format("Failed to hook function: {}", function_name));
        }
    }

    void swap_virtual_func(
        const void* class_ptr,
        const std::string& function_name,
        const uint16_t ordinal,
        const void* callback_function
    ) {
        try {
            swap_virtual_func_or_throw(
                class_ptr,
                function_name,
                ordinal,
                callback_function
            );
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

    void* get_hooked_function_address(const std::string& function_name) {
        if(not hook_map.contains(function_name)) {
            util::panic(
                std::format("Hook map does not contain function: {}", function_name)
            );
        }

        return hook_map.at(function_name).orig_func_ptr;
    }

    void* get_swapped_function_address(
        const void* class_ptr,
        const std::string& function_name
    ) {
        if(not class_map.contains(class_ptr)) {
            util::panic(
                std::format("Class hook map does not contain class: {}", class_ptr)
            );
        }

        const auto& function_map = class_map.at(class_ptr);

        if(not function_map.contains(function_name)) {
            util::panic(
                std::format("Function hook map does not contain function: {}", function_name)
            );
        }

        return function_map.at(function_name).orig_func_ptr;
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
