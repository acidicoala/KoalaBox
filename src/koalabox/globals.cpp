#include <koalabox/globals.hpp>
#include <koalabox/util.hpp>

namespace koalabox::globals {

    namespace {
        Mutex mutex;
        bool initialized = false;

        HMODULE self_handle = nullptr;
        String project_name = "";

        void validate_initialization() {
            MutexLockGuard lock(mutex);

            if (not initialized) {
                util::panic("Koalabox globals are not initialized. Call koalabox::globals::init_globals(...);");
            }
        }
    }

    KOALABOX_API(HMODULE) get_self_handle() {
        validate_initialization();

        return self_handle;
    }

    KOALABOX_API(String) get_project_name() {
        validate_initialization();
        
        return project_name;
    }

    KOALABOX_API(void) init_globals(HMODULE handle, String name) {
        MutexLockGuard lock(mutex);

        self_handle = handle;
        project_name = name;

        initialized = true;
    }

}
