#include "logger.hpp"

#include <spdlog/pattern_formatter.h>

using namespace koalabox;

auto logger::_instance = spdlog::null_logger_mt("null"); // NOLINT(cert-err58-cpp)

class EmojiFormatterFlag;

[[maybe_unused]] void logger::init(Path& path) {
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
        std::string some_txt;
        switch (log_msg.level) {
            case spdlog::level::critical:
                some_txt = "💥";
                break;
            case spdlog::level::err:
                some_txt = "❌";
                break;
            case spdlog::level::warn:
                some_txt = "⚠";
                break;
            case spdlog::level::info:
                some_txt = "ℹ";
                break;
            case spdlog::level::debug:
                some_txt = "⬛";
                break;
            case spdlog::level::trace:
                some_txt = "🔍";
                break;
            default:
                some_txt = " ";
                break;
        }

        dest.append(some_txt.data(), some_txt.data() + some_txt.size());
    }

    [[nodiscard]]
    std::unique_ptr<custom_flag_formatter> clone() const override {
        return spdlog::details::make_unique<EmojiFormatterFlag>();
    }
};
