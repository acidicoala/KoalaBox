#include <koalabox/logger.hpp>
#include <koalabox/util.hpp>

#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/async.h>

#include <regex>

namespace koalabox::logger {

    std::shared_ptr<spdlog::logger> instance = spdlog::null_logger_mt("null"); // NOLINT(cert-err58-cpp)

    /**
     * A custom implementation of a file sink which sanitizes messages before outputting them.
     */
    class SanitizedFileSink : public spdlog::sinks::base_sink<Mutex> {
    public:
        const spdlog::filename_t& filename() const {
            return file_helper_.filename();
        }

        explicit SanitizedFileSink(const spdlog::filename_t& filename) {
            file_helper_.open(filename, true);
        }

    protected:
        void sink_it_(const spdlog::details::log_msg& msg) override {
            // Replace username with environment variable
            static const std::regex username_regex(R"(\w:[/\\]Users[/\\]([^/\\]+))", std::regex_constants::icase);

            String payload(msg.payload.data(), msg.payload.size());
            std::smatch matches;
            if (std::regex_search(payload, matches, username_regex)) {
                payload.replace(matches[1].first, matches[1].second, "%USERNAME%");
            }

            const spdlog::details::log_msg sanitized_log_msg(msg.time, msg.source, msg.logger_name, msg.level, payload);
            spdlog::memory_buf_t formatted;
            base_sink<Mutex>::formatter_->format(sanitized_log_msg, formatted);

            file_helper_.write(formatted);
        }

        void flush_() override {
            file_helper_.flush();
        };

    private:
        spdlog::details::file_helper file_helper_;
    };

    class EmojiFormatterFlag : public spdlog::custom_flag_formatter {
    public:
        void format(const spdlog::details::log_msg& log_msg, const std::tm&, spdlog::memory_buf_t& dest) override {
            String emoji;
            switch (log_msg.level) {
                case spdlog::level::critical:
                    emoji = "💥";
                    break;
                case spdlog::level::err:
                    emoji = "🟥";
                    break;
                case spdlog::level::warn:
                    emoji = "🟨";
                    break;
                case spdlog::level::info:
                    emoji = "🟩";
                    break;
                case spdlog::level::debug:
                    emoji = "⬛";
                    break;
                case spdlog::level::trace:
                    emoji = "🟦";
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

    /**
     * @param path It is the responsibility of the caller to ensure that all directories in the path exist.
     */
    KOALABOX_API(void) init_file_logger(const Path& path) {
        instance = spdlog::create_async<SanitizedFileSink>("async", path.string());

        auto formatter = std::make_unique<spdlog::pattern_formatter>();
        formatter->add_flag<EmojiFormatterFlag>('*');
        formatter->set_pattern("%*│ %H:%M:%S.%e │%v");

        instance->set_formatter(std::move(formatter));
        instance->set_level(spdlog::level::trace);
        instance->flush_on(spdlog::level::trace);
    }

    KOALABOX_API(String) get_filename(const char* full_path) {
        static Map<const char*, String> results;

        if (not results.contains(full_path)) {
            results[full_path] = Path(full_path).filename().string();
        }

        return results[full_path];
    }
}
