#include <ranges>

#include <polyhook2/Detour/NatDetour.hpp>
#include <polyhook2/Virtuals/VFuncSwapHook.hpp>

#include "koalabox/hook.hpp"
#include "koalabox/globals.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/module.hpp"
#include "koalabox/path.hpp"
#include "koalabox/str.hpp"
#include "koalabox/util.hpp"

namespace {
    namespace kb = koalabox;

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

    struct hook_data_t {
        void* orig_func_ptr;
        // NOTE: It's important to keep that as raw pointers,
        // since wrapping them in unique_ptr makes the struct uncopiable,
        // which leads to a multitude of problems on clang.
        PLH::VFuncMap* vfunc_map; // We need to save this to support unhooking
        PLH::IHook* hook;
    };

    // Key is function name.
    using function_to_hook_data_map = std::map<std::string, hook_data_t>;

    // Used for vtable swap hooks. Key is class pointer.
    auto& get_class_map() {
        static std::map<const void*, function_to_hook_data_map> class_map;
        return class_map;
    }

    auto& get_reverse_class_map() {
        // Used as a fallback mechanism when functions from different classes
        // end up being hooked with the same function. Normally we would use
        // the class_map to get the class pointer first, but it may be missing
        // in cases like late injection/hooking. Hence, as a last-resort method,
        // we could try using the last known class pointer in the hopes that it
        // may be compatible. Key is function name.
        static std::map<std::string, const void*> reverse_class_map = {};
        return reverse_class_map;
    }

    // Used for detours/eat hooks. Key is function name.
    auto& get_hook_map() {
        static function_to_hook_data_map hook_map = {};
        return hook_map;
    }

    const function_to_hook_data_map& find_function_map(
        const void* class_ptr,
        const std::string& function_name
    ) {
        const auto& class_map = get_class_map();

        if(class_map.contains(class_ptr)) {
            return class_map.at(class_ptr);
        }
        LOG_ERROR(
            "Hook map does not contain class pointer: {}.\n"
            "Falling back to last known class pointer of {}.\n"
            "Most likely cause: {} has been loaded too late.",
            class_ptr,
            function_name,
            kb::globals::get_project_name()
        );

        const auto& reverse_class_map = get_reverse_class_map();
        if(reverse_class_map.contains(function_name)) {
            const auto* fallback_class_ptr = reverse_class_map.at(function_name);

            if(class_map.contains(fallback_class_ptr)) {
                return class_map.at(fallback_class_ptr);
            }

            kb::util::panic(
                std::format(
                    "Fallback class pointer '{}' was not found in the class map",
                    fallback_class_ptr
                )
            );
        }

        kb::util::panic(
            std::format("Function '{}' was not found in the reverse class map", function_name)
        );
    }
}

namespace koalabox::hook {
    bool is_hooked(const std::string& function_name) {
        return get_hook_map().contains(function_name);
    }

    bool is_vt_hooked(const void* class_ptr, const std::string& function_name) {
        const auto& class_map = get_class_map();

        return class_map.contains(class_ptr) &&
               class_map.at(class_ptr).contains(function_name);
    }

    bool unhook(const std::string& function_name) {
        static std::mutex section;
        const std::lock_guard lock(section);

        auto& hook_map = get_hook_map();

        if(not hook_map.contains(function_name)) {
            LOG_ERROR("Cannot unhook '{}'. Function name not found", function_name);
            return false;
        }

        const auto& hook_data = hook_map.at(function_name);
        const auto success = hook_data.hook->unHook();
        delete hook_data.hook;
        delete hook_data.vfunc_map;
        hook_map.erase(function_name);

        LOG_DEBUG("{} -> Unhooked '{}'", __func__, function_name);

        return success;
    }

    bool unhook_vt(const void* class_ptr, const std::string& function_name) {
        static std::mutex section;
        const std::lock_guard lock(section);

        auto& class_map = get_class_map();

        if(not class_map.contains(class_ptr)) {
            LOG_ERROR("Cannot unhook '{}'. Class pointer not found: {}", function_name, class_ptr)
            return false;
        }

        auto& function_map = class_map.at(class_ptr);

        if(not function_map.contains(function_name)) {
            LOG_ERROR("Cannot unhook '{}'. Function name not found", function_name);
            return false;
        }

        const auto& hook_data = function_map.at(function_name);
        const auto success = hook_data.hook->unHook();
        delete hook_data.hook;
        delete hook_data.vfunc_map;
        function_map.erase(function_name);
        get_reverse_class_map().erase(function_name);

        LOG_DEBUG("{} -> Unhooked '{}' from {}", __func__, function_name, class_ptr);

        return success;
    }

    bool unhook_vt_all(const void* class_ptr) {
        static std::mutex section;
        const std::lock_guard lock(section);

        auto& class_map = get_class_map();

        if(not class_map.contains(class_ptr)) {
            LOG_ERROR("Unhooking error. Class pointer not found: {}", class_ptr)
            return false;
        }

        // This should call the destructor of hooked functions, which will unhook them.
        class_map.erase(class_ptr);

        LOG_DEBUG("{} -> Unhooked all functions from {}", __func__, class_ptr);

        return true;
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

        // False flag - no memory is actually leaked, it is freed in unhook
        // ReSharper disable once CppDFAMemoryLeak
        auto* const detour = new PLH::NatDetour(
            reinterpret_cast<uint64_t>(address),
            reinterpret_cast<uint64_t>(callback_function),
            &trampoline
        );

#ifdef _WIN64
        detour->setDetourScheme(PLH::x64Detour::ALL);
#endif
        if(detour->hook()) {
            get_hook_map()[function_name] = {
                .orig_func_ptr = reinterpret_cast<void*>(trampoline),
                .vfunc_map = new PLH::VFuncMap{},
                .hook = detour,
            };
        } else {
            delete detour;
            throw std::runtime_error(std::format("Failed to hook function: {}", function_name));
        }
    }

    void detour_module_or_throw(
        void* const module_handle,
        const std::string& function_name,
        const void* callback_function
    ) {
        const auto* address = module::get_function_address(module_handle, function_name.c_str()).value();

        detour_or_throw(address, function_name, callback_function);
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

    void detour_module_or_warn(
        const HMODULE& module_handle,
        const std::string& function_name,
        const void* callback_function
    ) {
        try {
            detour_module_or_throw(module_handle, function_name, callback_function);
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

    void detour_module(
        const HMODULE& module_handle,
        const std::string& function_name,
        const void* callback_function
    ) {
        try {
            detour_module_or_throw(module_handle, function_name, callback_function);
        } catch(const std::exception& ex) {
            util::panic(
                std::format("Failed to hook function {} via Detour: {}", function_name, ex.what())
            );
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

        // False positive - No memory is actually leaked, it is freed in unhook_vt
        // ReSharper disable once CppDFAMemoryLeak
        auto* original_functions = new PLH::VFuncMap();

        // False positive - No memory is actually leaked, it is freed in unhook_vt
        // ReSharper disable once CppDFAMemoryLeak
        auto* const swap_hook = new PLH::VFuncSwapHook(
            static_cast<const char*>(class_ptr),
            redirect,
            original_functions
        );

        if(swap_hook->hook()) {
            get_class_map()[class_ptr][function_name] = {
                .orig_func_ptr = reinterpret_cast<void*>((*original_functions)[ordinal]),
                .vfunc_map = original_functions,
                .hook = swap_hook,
            };
            get_reverse_class_map()[function_name] = class_ptr;
        } else {
            delete swap_hook;
            delete original_functions;
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
        const auto& hook_map = get_hook_map();

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
        const auto& function_map = find_function_map(class_ptr, function_name);

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

    bool is_hook_mode(const HMODULE self_module, const std::string& orig_library_name) {
        // E.g. C:/Library/api.dll
        const auto module_path = module::get_fs_path(self_module);

        // E.g. api
        const auto self_name = path::to_str(module_path.stem());

        return not str::eq(self_name, orig_library_name);
    }
}
