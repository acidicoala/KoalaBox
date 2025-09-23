#pragma once

#include <string>

// TODO: Move to core
namespace koalabox::globals {
    /**
     * @return A handle representing the project DLL. Usually obtained from DllMain.
     */
    void* get_self_handle();

    const std::string& get_project_name();

    void init_globals(void* handle, const std::string& name);
}