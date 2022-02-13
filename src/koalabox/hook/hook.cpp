#include "hook.hpp"
#include "koalabox/koalabox.hpp"
#include "koalabox/util/util.hpp"
#include "koalabox/logger/logger.hpp"

#include <polyhook2/CapstoneDisassembler.hpp>
#include <polyhook2/Detour/x86Detour.hpp>
#include <polyhook2/Detour/x64Detour.hpp>

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

    Map<String, uint64_t> address_book; // NOLINT(cert-err58-cpp)
    Vector<Detour*> hooks; // NOLINT(cert-err58-cpp)

    [[maybe_unused]]
    void detour(HMODULE module, const char* function_name, FunctionPointer callback_function) {
        logger::debug("Hooking '{}'", function_name);

        static PLH::CapstoneDisassembler disassembler(
            util::is_64_bit()
                ? PLH::Mode::x64
                : PLH::Mode::x86
        );

        const auto address = (FunctionPointer) win_util::get_proc_address(module, function_name);

        uint64_t trampoline;

        auto detour = new Detour(address, callback_function, &trampoline, disassembler);

        address_book[function_name] = trampoline;

        if (detour->hook()) {
            hooks.push_back(detour);
        } else {
            util::panic(__func__, "Failed to hook function: {}", function_name);
        }
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

