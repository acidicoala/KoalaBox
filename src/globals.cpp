#include "koalabox/globals.hpp"
#include "koalabox/util.hpp"

#ifdef _DEBUG
#define VALIDATE_INIT()                                                                            \
    if (not initialized) {                                                                         \
        throw std::runtime_error("Koalabox not initialized");                                      \
    }
#endif

namespace koalabox::globals {

    namespace {
        bool initialized = false;

        HMODULE self_handle = nullptr;
        std::string project_name;
    }

    HMODULE get_self_handle() {
        VALIDATE_INIT();

        return self_handle;
    }

    std::string get_project_name() {
        VALIDATE_INIT();

        return project_name;
    }

    void init_globals(const HMODULE handle, const std::string& name) {
        self_handle = handle;
        project_name = name;

        initialized = true;

        DisableThreadLibraryCalls(self_handle);
    }

}
