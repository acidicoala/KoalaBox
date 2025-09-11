#include <inja/inja.hpp>

#include "config.hpp"

namespace {
    std::string replace_variables(const std::string& str) {
        return inja::render(str, config::options.get_variables());
    }
}

namespace config {
    std::string TextTask::get_template_file() const {
        return replace_variables(template_file);
    }

    std::string TextTask::get_destination_dir() const {
        return replace_variables(destination_dir);
    }

    std::string TextTask::get_file_name() const {
        return replace_variables(file_name);
    }

    std::string JsonTask::get_schema_file() const {
        return replace_variables(schema_file);
    }

    std::string JsonTask::get_destination_file() const {
        return replace_variables(destination_file);
    }

    const std::vector<Task>& Config::get_tasks() const {
        return tasks;
    }

    const nlohmann::json& Config::get_variables() const {
        return variables;
    }

    Config options;
}
