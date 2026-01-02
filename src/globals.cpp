#include "koalabox/globals.hpp"
#include "koalabox/util.hpp"

#ifdef _DEBUG
#include <stdexcept>
#define VALIDATE_INIT()                                                                            \
    if (not initialized) {                                                                         \
        throw std::runtime_error("Koalabox not initialized");                                      \
    }
#else
#define VALIDATE_INIT()
#endif

namespace koalabox::globals {
    namespace {
        bool initialized = false;

        void* self_handle = nullptr;
        std::string project_name;
    }

    void* get_self_handle() {
        VALIDATE_INIT();

        return self_handle;
    }

    const std::string& get_project_name() {
        VALIDATE_INIT();

        return project_name;
    }

    void init_globals(void* handle, const std::string& name) {
        self_handle = handle;
        project_name = name;

        initialized = true;

#ifdef KB_WIN
        DisableThreadLibraryCalls(static_cast<HMODULE>(self_handle));
#endif
    }
}
