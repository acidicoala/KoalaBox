#pragma once

#include <koalabox/core.hpp>

namespace koalabox::globals {

    /**
     * @return A handle representing the project DLL. Usually obtained from DllMain.
     */
    KOALABOX_API(HMODULE) get_self_handle();

    KOALABOX_API(String) get_project_name(bool validate = true);

    KOALABOX_API(void) init_globals(HMODULE handle, String name);

}
