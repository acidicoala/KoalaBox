#include "logger.hpp"
#include "koalabox/koalabox.hpp"

namespace koalabox::logger {
    using namespace koalabox;

    class EmojiFormatterFlag : public spdlog::custom_flag_formatter {
    public:
        void format(const spdlog::details::log_msg& log_msg, const std::tm&, spdlog::memory_buf_t& dest) override {
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

    std::shared_ptr<spdlog::logger> create(const Path& path) {
        auto logger = spdlog::basic_logger_st("default", path.string(), true);

        auto formatter = std::make_unique<spdlog::pattern_formatter>();
        formatter->add_flag<EmojiFormatterFlag>('*').set_pattern("[%H:%M:%S.%e] %* ┃ %v");

        logger->set_formatter(std::move(formatter));
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::trace);

        return logger;
    }
}
