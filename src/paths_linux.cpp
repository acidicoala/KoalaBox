#include <pwd.h>
#include <unistd.h>

#include "koalabox.hpp"

namespace koalabox::paths {
    fs::path get_user_dir() {
        if (const char* home = std::getenv("HOME")) {
            return path::from_str(home);
        }

        if (const auto* pwd = getpwuid(getuid())) {
            return path::from_str(pwd->pw_dir);
        }

        throw std::runtime_error("Failed to get user directory");
    }
}
