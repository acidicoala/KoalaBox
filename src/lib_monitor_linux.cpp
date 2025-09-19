#include <ranges>
#include <set>

#include <dlfcn.h>

#include "koalabox/hook.hpp"
#include "koalabox/lib_monitor.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/path.hpp"

#define HOOK(FUNC) hook::detour(reinterpret_cast<void*>(FUNC), #FUNC, reinterpret_cast<void*>($##FUNC))

namespace {
    namespace kb = koalabox;
    using namespace kb::lib_monitor;

    void* $dlopen(const char* filename, const int flag) {
        static const auto dlopen$ = KB_HOOK_GET_HOOKED_FN(dlopen);
        auto* const lib_handle = dlopen$(filename, flag);

        details::on_library_loaded(filename, lib_handle);

        return lib_handle;
    }

    void* $dlmopen(const Lmid_t namespace_id, const char* filename, const int flag) {
        static const auto dlmopen$ = KB_HOOK_GET_HOOKED_FN(dlmopen);
        auto* const lib_handle = dlmopen$(namespace_id, filename, flag);

        details::on_library_loaded(filename, lib_handle);

        return lib_handle;
    }
}

namespace koalabox::lib_monitor {
    bool is_initialized() {
        return hook::is_hooked("dlopen") || hook::is_hooked("dlmopen");
    }

    namespace details {
        void init() {
            HOOK(dlopen);
            HOOK(dlmopen);
        }

        void shutdown() {
            hook::unhook("dlopen");
            hook::unhook("dlmopen");
        }
    }
}
