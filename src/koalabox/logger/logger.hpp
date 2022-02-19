#pragma once

#include "koalabox/koalabox.hpp"

namespace koalabox::logger {

    std::shared_ptr<spdlog::logger> create(const Path& path);

}
