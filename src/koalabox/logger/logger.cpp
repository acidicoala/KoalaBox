#include "logger.hpp"

#include <spdlog/pattern_formatter.h>

using namespace koalabox;

logger::Logger logger::_instance = spdlog::null_logger_mt("null"); // NOLINT(cert-err58-cpp)

class EmojiFormatterFlag;

[[maybe_unused]] void logger::init(Path path) { // NOLINT(performance-unnecessary-value-param)
    _instance = spdlog::basic_logger_mt("default", path.string(), true);

    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<EmojiFormatterFlag>('*').set_pattern("[%H:%M:%S.%e] %* ┃ %v");

    _instance->set_formatter(std::move(formatter));
    _instance->set_level(spdlog::level::trace);
    _instance->flush_on(spdlog::level::debug);
}

class EmojiFormatterFlag : public spdlog::custom_flag_formatter {
public:
    void format(
        const spdlog::details::log_msg& log_msg,
        const std::tm&,
        spdlog::memory_buf_t& dest
    ) override {
        String emoji;
        switch (log_msg.level) {
            case spdlog::level::critical:
                emoji = "💥";
                break;
            case spdlog::level::err:
                emoji = "❌";
                break;
            case spdlog::level::warn:
                emoji = "⚠";
                break;
            case spdlog::level::info:
                emoji = "ℹ";
                break;
            case spdlog::level::debug:
                emoji = "⬛";
                break;
            case spdlog::level::trace:
                emoji = "🔍";
                break;
            default:
                emoji = " ";
                break;
        }

        dest.append(emoji.data(), emoji.data() + emoji.size());
    }

    [[nodiscard]]
    std::unique_ptr<custom_flag_formatter> clone() const override {
        return spdlog::details::make_unique<EmojiFormatterFlag>();
    }
};
