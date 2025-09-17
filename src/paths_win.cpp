#include <ShlObj.h>

#include "koalabox/paths.hpp"
#include "koalabox/win.hpp"

namespace koalabox::paths {
    fs::path get_user_dir() {
        TCHAR buffer[MAX_PATH];
        if(SHGetSpecialFolderPath(nullptr, buffer, CSIDL_PROFILE, FALSE)) {
            // Path retrieved successfully, so print it out
            return {buffer};
        }

        throw std::runtime_error(
            std::format("Error retrieving user directory: {}", win::get_last_error())
        );
    }
}
