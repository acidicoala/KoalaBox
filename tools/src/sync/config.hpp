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
    private:
        TaskType type = TaskType::Text;
        std::string template_file;
        std::string destination_dir;
        std::string file_name;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(TextTask, type, template_file, destination_dir, file_name);

    public:
        [[nodiscard]] std::string get_template_file() const;
        [[nodiscard]] std::string get_destination_dir() const;
        [[nodiscard]] std::string get_file_name() const;
    };

    struct JsonTask {
    private:
        TaskType type = TaskType::Json;
        std::string schema_file;
        std::string destination_file;
        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(JsonTask, type, schema_file, destination_file);

    public:
        [[nodiscard]] std::string get_schema_file() const;
        [[nodiscard]] std::string get_destination_file() const;
    };

    using Task = std::variant<TextTask, JsonTask>;

    struct Config {
    private:
        nlohmann::json variables;
        nlohmann::ordered_json resolved_variables;
        std::vector<Task> tasks;
        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Config, variables, tasks);

    public:
        [[nodiscard]] const std::vector<Task>& get_tasks() const;
        [[nodiscard]] const nlohmann::json& get_variables() const;
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
        }
    }

    static void to_json(json& j, const config::Task& generator) {
        if(std::holds_alternative<config::TextTask>(generator)) {
            j = std::get<config::TextTask>(generator);
        } else if(std::holds_alternative<config::JsonTask>(generator)) {
            j = std::get<config::JsonTask>(generator);
        }
    }
};
