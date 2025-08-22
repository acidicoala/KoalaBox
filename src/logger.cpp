#include <regex>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#include "koalabox/logger.hpp"
#include "koalabox/paths.hpp"

namespace {
    namespace fs = std::filesystem;

    std::string get_logger_pattern() {
        // See https://github.com/gabime/spdlog/wiki/Custom-formatting

        // Potentially useful flags:
        // %P - Process ID

        // %t - Thread ID
        // %s - Basename of the source file
        // %# - Source line
        // %! - Source function
        // %H - Hours in 24 format 00-23
        // %M - Minutes 00-59
        // %S - Seconds 00-59
        // %e - Millisecond part of the current second 000-999
        // %v - The actual text to log

        // align            meaning                     example result
        // %<width>!<flag>  Right align or truncate	%3!l	"inf"
        // %-<width>!<flag> Left align or truncate	%-2!l	"in"
        // %=<width>!<flag> Center align or truncate    %=1!l	"i"

        // Functions names unfortunately have to be excluded because they
        // contain annoying prefixes like `anonymous-namespace::`
        // constexpr auto src_func_name = "%-32!!";

        constexpr auto src_file_name = "%32!s";
        constexpr auto src_line_num = "%-4!#";
        constexpr auto timestamp = "%H:%M:%S.%e";
        constexpr auto thread_id = "%6!t";

        return std::format("%L|{}|{}:{}|{}|%v", timestamp, src_file_name, src_line_num, thread_id);
    }

    class UsernameFilterFormatter final : public spdlog::formatter {
    public:
        void format(const spdlog::details::log_msg& msg, spdlog::memory_buf_t& dest) override {
            static const std::regex username_regex(
                R"(\w:[/\\]Users[/\\]([^/\\]+))",
                std::regex_constants::icase
            );

            std::string filtered_msg(msg.payload.data(), msg.payload.size());
            if(std::smatch matches; std::regex_search(filtered_msg, matches, username_regex)) {
                filtered_msg.replace(matches[1].first, matches[1].second, "%USERNAME%");
            }

            fmt::format_to(std::back_inserter(dest), "{}", filtered_msg);
        }

        std::unique_ptr<formatter> clone() const override {
            return spdlog::details::make_unique<UsernameFilterFormatter>();
        }
    };

    void configure_logger(const std::shared_ptr<spdlog::logger>& logger) {
        auto formatter = std::make_unique<UsernameFilterFormatter>();
        logger->set_formatter(std::move(formatter));
        logger->set_pattern(get_logger_pattern());
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::trace);

        spdlog::set_default_logger(logger);
    }
}

namespace koalabox::logger {
    void init_file_logger(const fs::path& path) {
        fs::create_directories(path.parent_path());

        const auto logger = spdlog::basic_logger_mt("file", path.string(), true);
        configure_logger(logger);
    }

    void init_console_logger() {
        const auto logger = spdlog::stdout_logger_mt("console");
        configure_logger(logger);
    }

    void shutdown() {
        spdlog::shutdown();
    }
}