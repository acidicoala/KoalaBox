#include "hook.hpp"
#include "koalabox/koalabox.hpp"
#include "koalabox/util/util.hpp"
#include "koalabox/logger/logger.hpp"

#include <polyhook2/CapstoneDisassembler.hpp>
#include <polyhook2/Detour/x86Detour.hpp>
#include <polyhook2/Detour/x64Detour.hpp>
#include <polyhook2/PE/EatHook.hpp>

namespace koalabox::hook {
    using namespace koalabox;

    class PolyhookLogger : public PLH::Logger {
        void log(String msg, PLH::ErrorLevel level) override {
            if (level == PLH::ErrorLevel::WARN) {
                logger::warn("[Polyhook] {}", msg);
            } else if (level == PLH::ErrorLevel::SEV) {
                logger::error("[Polyhook] {}", msg);
            }
        }
    };

#ifdef _WIN64
    typedef PLH::x64Detour Detour;
#else
    typedef PLH::x86Detour Detour;
#endif

    Map <String, FunctionPointer> address_book; // NOLINT(cert-err58-cpp)

    Vector<PLH::IHook*> hooks; // NOLINT(cert-err58-cpp)

    [[maybe_unused]]
    bool eat_hook(
        HMODULE module,
        const String& function_name,
        FunctionPointer callback_function,
        bool panic_on_fail
    ) {
        logger::debug("Hooking '{}' via EAT", function_name);

        // TODO: Add support for absolute paths / module handles
        const auto module_path = Path(win_util::get_module_file_name(module));

        uint64_t orig_function_address = 0;
        auto eat_hook = new PLH::EatHook(
            function_name,
            util::to_wstring(module_path.filename().string()),
            (char*) callback_function,
            &orig_function_address
        );

        if (eat_hook->hook()) {
            address_book[function_name] = reinterpret_cast<FunctionPointer>(orig_function_address);

            hooks.push_back(eat_hook);

            return true;
        } else {
            const auto message = fmt::format("Failed to hook function: '{}'", function_name);

            if (panic_on_fail) {
                util::panic(__func__, message);
            } else {
                logger::error(message);
            }

            return false;
        }
    }

    [[maybe_unused]]
    bool detour(
        HMODULE module,
        const String& function_name,
        FunctionPointer callback_function,
        bool panic_on_fail
    ) {
        logger::debug("Hooking '{}' via Detour", function_name);

        static PLH::CapstoneDisassembler disassembler(
            util::is_64_bit()
                ? PLH::Mode::x64
                : PLH::Mode::x86
        );

        const auto address = reinterpret_cast<FunctionPointer>(
            ::GetProcAddress(module, function_name.c_str())
        );

        if (not address) {
            const auto message = fmt::format("Failed to get function address: {}", function_name);
            if (panic_on_fail) {
                util::panic(__func__, message);
            } else {
                logger::error(message);
            }
            return false;
        }

        uint64_t trampoline = 0;

        auto detour = new Detour(address, callback_function, &trampoline, disassembler);

#ifdef _WIN64
        detour->setDetourScheme(Detour::RECOMMENDED);
#endif

        if (detour->hook()) {
            address_book[function_name] = reinterpret_cast<FunctionPointer>(trampoline);

            hooks.push_back(detour);
            return true;
        } else {
            const auto message = fmt::format("Failed to hook function: {}", function_name);

            if (panic_on_fail) {
                util::panic(__func__, message);
            } else {
                logger::error(message);
            }
            return false;
        }
    }

    [[maybe_unused]]
    void detour_with_fallback(
        HMODULE module,
        const String& function_name,
        FunctionPointer callback_function,
        bool panic_on_fail
    ) {
        if(detour(module, function_name, callback_function, false)){
            return;
        }

        eat_hook(module, function_name, callback_function, panic_on_fail);
    }

    [[maybe_unused]]
    void init(const std::function<void()>& callback) {
        logger::debug("Hooker initialization");

        // Initialize polyhook logger
        auto polyhook_logger = std::make_shared<PolyhookLogger>();
        PLH::Log::registerLogger(polyhook_logger);

        callback();
    }


    [[maybe_unused]]
    bool is_hook_mode(
        HMODULE self_module,
        const String& orig_library_name
    ) {
        const auto module_path = win_util::get_module_file_name(self_module);

        const auto self_name = Path(module_path).filename().string();

        return not util::strings_are_equal(self_name, orig_library_name + ".dll");
    }
}
