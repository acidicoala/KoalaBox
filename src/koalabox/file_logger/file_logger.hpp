#pragma once

#include "koalabox/koalabox.hpp"

namespace koalabox::file_logger {

    std::shared_ptr<spdlog::logger> create(const Path& path);

}
