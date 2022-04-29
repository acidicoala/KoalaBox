#include <koalabox/hook.hpp>
#include <koalabox/loader.hpp>
#include <koalabox/win_util.hpp>
#include <polyhook2/Virtuals/VFuncSwapHook.hpp>
#include <polyhook2/PE/EatHook.hpp>

#ifdef _WIN64
#include <polyhook2/Detour/x64Detour.hpp>
#else
#include <polyhook2/Detour/x86Detour.hpp>
#endif

namespace koalabox::hook {
    using namespace koalabox;

    class PolyhookLogger : public PLH::Logger {
        bool print_info;
    public:
        explicit PolyhookLogger(bool print_info) : print_info(print_info) {};

        void log(String msg, PLH::ErrorLevel level) override {
            if (level == PLH::ErrorLevel::INFO && print_info) {
                koalabox::logger->debug("[Polyhook] {}", msg);
            } else if (level == PLH::ErrorLevel::WARN) {
                koalabox::logger->warn("[Polyhook] {}", msg);
            } else if (level == PLH::ErrorLevel::SEV) {
                koalabox::logger->error("[Polyhook] {}", msg);
            }
        }
    };

#ifdef _WIN64
    typedef PLH::x64Detour Detour;
#else
    typedef PLH::x86Detour Detour;
#endif

    Map<String, FunctionAddress> address_book; // NOLINT(cert-err58-cpp)

    Vector<PLH::IHook*> hooks; // NOLINT(cert-err58-cpp)

    void detour_or_throw(
        const FunctionAddress address,
        const String &function_name,
        const FunctionAddress callback_function
    ) {
        logger->debug("Hooking '{}' at {} via Detour", function_name, (void*) address);

        uint64_t trampoline = 0;

        auto* const detour = new Detour(address, callback_function, &trampoline);

#ifdef _WIN64
        detour->setDetourScheme(Detour::ALL);
#endif
        if (detour->hook()) {
            address_book[function_name] = trampoline;

            hooks.push_back(detour);
        } else {
            throw util::exception("Failed to hook function: {}", function_name);
        }
    }

    void detour_or_throw(const HMODULE &module, const String &function_name, const FunctionAddress callback_function) {
        const auto address = reinterpret_cast<FunctionAddress>(
            win_util::get_proc_address_or_throw(module, function_name.c_str())
        );

        detour_or_throw(address, function_name, callback_function);
    }

    void detour_or_warn(const HMODULE &module, const String &function_name, const FunctionAddress callback_function) {
        try {
            hook::detour_or_throw(module, function_name, callback_function);
        } catch (const Exception &ex) {
            logger->warn(ex.what());
        }
    }

    void detour(const HMODULE &module, const String &function_name, const FunctionAddress callback_function) {
        try {
            detour_or_throw(module, function_name, callback_function);
        } catch (const Exception &ex) {
            util::panic("Failed to hook function {} via Detour: {}", function_name, ex.what());
        }
    }

    void detour(
        const FunctionAddress address,
        const String &function_name,
        const FunctionAddress callback_function
    ) {
        try {
            detour_or_throw(address, function_name, callback_function);
        } catch (const Exception &ex) {
            util::panic("Failed to hook function {} via Detour: {}", function_name, ex.what());
        }
    }

    void eat_hook_or_throw(
        const HMODULE &module,
        const String &function_name,
        FunctionAddress callback_function
    ) {
        logger->debug("Hooking '{}' via EAT", function_name);

        uint64_t orig_function_address = 0;
        auto* const eat_hook = new PLH::EatHook(
            function_name,
            module,
            callback_function,
            &orig_function_address
        );

        if (eat_hook->hook()) {
            address_book[function_name] = orig_function_address;

            hooks.push_back(eat_hook);
        } else {
            delete eat_hook;

            throw util::exception("Failed to hook function: '{}'", function_name);
        }
    }

    void eat_hook_or_warn(
        const HMODULE &module,
        const String &function_name,
        FunctionAddress callback_function
    ) {
        try {
            hook::eat_hook_or_throw(module, function_name, callback_function);
        } catch (const Exception &ex) {
            logger->warn(ex.what());
        }
    }

    void swap_virtual_func_or_throw(
        const void* instance,
        const String &function_name,
        const int ordinal,
        FunctionAddress callback_function
    ) {
        logger->debug("Hooking '{}' at {} via virtual function swap", function_name, instance);

        PLH::VFuncMap redirect = {
            {ordinal, callback_function},
        };

        PLH::VFuncMap original_functions;
        auto* const swap = new PLH::VFuncSwapHook((char*) instance, redirect, &original_functions);

        if (swap->hook()) {
            address_book[function_name] = original_functions[ordinal];

            hooks.push_back(swap);
        } else {
            throw util::exception("Failed to hook function: {}", function_name);
        }
    }

    void swap_virtual_func(
        const void* instance,
        const String &function_name,
        const int ordinal,
        FunctionAddress callback_function
    ) {
        try {
            swap_virtual_func_or_throw(instance, function_name, ordinal, callback_function);
        } catch (const Exception &ex) {
            util::panic("Failed to hook function {} via virtual function swap: {}", function_name, ex.what());
        }
    }

    FunctionAddress get_original_function(bool is_hook_mode, const HMODULE &library, const String &function_name) {
        if (is_hook_mode) {
            if (not hook::address_book.contains(function_name)) {
                util::panic("Address book does not contain function: {}", function_name);
            }

            return hook::address_book[function_name];
        }

        return reinterpret_cast<FunctionAddress>(
            win_util::get_proc_address(library, function_name.c_str())
        );
    }

    void init(bool print_info) {
        logger->debug("Hooking initialization");

        // Initialize polyhook logger
        auto polyhook_logger = std::make_shared<PolyhookLogger>(print_info);
        PLH::Log::registerLogger(polyhook_logger);
    }

    bool is_hook_mode(const HMODULE &self_module, const String &orig_library_name) {
        const auto module_path = win_util::get_module_file_name(self_module);

        const auto self_name = Path(module_path).filename().string();

        return not util::strings_are_equal(self_name, orig_library_name + ".dll");
    }
}
