#pragma once

#include <string>

namespace koalabox::globals {
    /**
     * @return A handle representing the project DLL. Usually obtained from DllMain.
     */
    HMODULE get_self_handle();

    std::string get_project_name();

    void init_globals(HMODULE handle, const std::string& name);
}
