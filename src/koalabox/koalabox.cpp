#include <koalabox/koalabox.hpp>

#include <spdlog/sinks/null_sink.h>

namespace koalabox {

    String project_name = "KoalaBox"; // NOLINT(cert-err58-cpp)

    std::shared_ptr<spdlog::logger> logger = spdlog::null_logger_st("null"); // NOLINT(cert-err58-cpp)

}
