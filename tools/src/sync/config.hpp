#pragma once

#include <variant>

#include <nlohmann/json.hpp>

namespace config {
    enum class TaskType {
        Text,
        Json,
        Markdown
    };

    // @formatter:off
    NLOHMANN_JSON_SERIALIZE_ENUM(TaskType, { // NOLINT(*-use-internal-linkage)
        { TaskType::Text,     "text" },
        { TaskType::Json,     "json" },
        { TaskType::Markdown, "markdown" },
    }); // @formatter:on

    struct TextTask {
        TaskType type = TaskType::Text;
    private:
        std::string template_file;
        std::string destination_dir;
        std::string file_name;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
            TextTask,
            type, template_file, destination_dir, file_name
        );
    public:
        std::string get_template_file() const;
        std::string get_destination_dir() const;
        std::string get_file_name() const;
    };

    struct JsonTask {
        TaskType type = TaskType::Json;
    private:
        std::string schema_file;
        std::string destination_file;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
            JsonTask,
            type, schema_file, destination_file
        );
    public:
        std::string get_schema_file() const;
        std::string get_destination_file() const;
    };

    struct MarkdownTask {
        TaskType type = TaskType::Markdown;
        std::string file_path;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
            MarkdownTask,
            type, file_path
        );
    };

    using Task = std::variant<TextTask, JsonTask, MarkdownTask>;

    struct Config {
        std::string templates_dir;
        std::map<std::string, std::string> variables;
        std::vector<Task> tasks;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
            Config,
            templates_dir, variables, tasks
        );
    };

    extern Config options;
}

template<>
struct nlohmann::adl_serializer<config::Task> {
    static void from_json(const json& j, config::Task& generator) {
        if(!j.contains("type")) {
            return;
        }

        const config::TaskType type = j["type"];

        if(type == config::TaskType::Text) {
            generator = config::TextTask(j);
        } else if(type == config::TaskType::Json) {
            generator = config::JsonTask(j);
        } else if(type == config::TaskType::Markdown) {
            generator = config::MarkdownTask(j);
        }
    }

    static void to_json(json& j, const config::Task& generator) {
        if(std::holds_alternative<config::TextTask>(generator)) {
            j = std::get<config::TextTask>(generator);
        } else if(std::holds_alternative<config::JsonTask>(generator)) {
            j = std::get<config::JsonTask>(generator);
        } else {
            j = std::get<config::MarkdownTask>(generator);
        }
    }
};
