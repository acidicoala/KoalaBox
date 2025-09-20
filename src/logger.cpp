#include <regex>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/null_sink.h>

#include "koalabox/logger.hpp"
#include "koalabox/path.hpp"

namespace {
    std::string get_logger_pattern() {
        // See https://github.com/gabime/spdlog/wiki/Custom-formatting

        // Potentially useful flags:
        // %P - Process ID

        // %t - Thread ID
        // %s - Basename of the source file
        // %# - Source line
        // %! - Source function
        // %T - ISO 8601 time format (HH:MM:SS)
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
        constexpr auto timestamp = "%T.%e";
        constexpr auto thread_id = "%6!t";

        return std::format("%-8l|{}|{}:{}|{}|%v", timestamp, src_file_name, src_line_num, thread_id);
    }

    // TODO: This is not working when SPDLOG_USE_STD_FORMAT is set
    class UsernameFilterFormatter final : public spdlog::formatter {
    public:
        // Redact usernames
        void format(const spdlog::details::log_msg& msg, spdlog::memory_buf_t& dest) override {
            // TODO: Linux support
            static const std::regex username_regex(R"(\w:[/\\]Users[/\\]([^/\\]+))", std::regex_constants::icase);

            std::string filtered_msg(msg.payload.data(), msg.payload.size());
            if(std::smatch matches; std::regex_search(filtered_msg, matches, username_regex)) {
                filtered_msg.replace(matches[1].first, matches[1].second, "%USERNAME%");
            }

            // Special case: Capitalize the first letter of log level
            filtered_msg[0] = static_cast<char>(std::toupper(filtered_msg[0]));

            std::format_to(std::back_inserter(dest), "{}", filtered_msg);
        }

        [[nodiscard]] std::unique_ptr<formatter> clone() const override {
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
    void init_file_logger(const std::filesystem::path& log_path) {
        std::filesystem::create_directories(log_path.parent_path());

        const auto logger = spdlog::basic_logger_mt("file", path::to_platform_str(log_path), true);

#ifdef KB_DEBUG
        // Useful for viewing logs directly in IDE console
        auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
        logger->sinks().emplace_back(console_sink);
#endif

        configure_logger(logger);
    }

    void init_console_logger() {
        const auto logger = spdlog::stdout_logger_mt("console");
        configure_logger(logger);
    }

    void init_null_logger() {
        const auto logger = spdlog::null_logger_mt("null");
        spdlog::set_default_logger(logger);
    }

    void shutdown() {
        spdlog::shutdown();
    }
}
