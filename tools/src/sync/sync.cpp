#include <filesystem>
#include <utility>
#include <variant>

#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

#include <koalabox/io.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/path.hpp>

#include "config.hpp"

namespace {
    namespace kb = koalabox;

    std::string read_file_safely(const fs::path& file_path) {
        if(!fs::exists(file_path) || fs::is_directory(file_path)) {
            LOG_ERROR("template_file not found: '{}'", kb::path::to_str(file_path));
            exit(1);
        }

        try {
            return kb::io::read_file(file_path);
        } catch(const std::exception& e) {
            LOG_ERROR("Failed to read file {}. Cause: {}", kb::path::to_str(file_path), e.what());
            exit(1);
        }
    }

    void generate_text(const config::TextTask& task) {
        const auto template_path = kb::path::from_str(config::options.templates_dir) / task.get_template_file();
        const auto template_str = read_file_safely(template_path);

        std::string rendered_str;
        try {
            rendered_str = inja::render(template_str, config::options.variables);
        } catch(const std::exception& e) {
            LOG_ERROR("Error parsing template {}: {}", task.get_template_file(), e.what());
            exit(1);
        }

        const auto output_dir = fs::absolute(
            kb::path::from_str(task.get_destination_dir().empty() ? "." : task.get_destination_dir())
        );
        const auto output_file = task.get_file_name().empty()
                                     ? kb::path::to_str(kb::path::from_str(task.get_template_file()).filename())
                                     : task.get_file_name();
        const auto output_path = output_dir / output_file;
        kb::io::write_file(output_path, rendered_str);
    }

    void generate_json(const config::JsonTask& task) {
        const auto schema_str = read_file_safely(task.get_schema_file());
        const auto schema_json = nlohmann::ordered_json::parse(schema_str);

        nlohmann::ordered_json output;
        for(const auto& [property, fields] : schema_json["properties"].items()) {
            if(property == "$schema") {
                // Special case
                output[property] = schema_json.at("$id");
                continue;
            }

            const std::vector<std::string> names = {"x-packaged-default", "default", "const"};

            for(const auto& name : names) {
                if(fields.contains(name)) {
                    output[property] = fields.at(name);
                    break;
                }
            }

            if(!output.contains(property)) {
                LOG_WARN("JSON Schema property has no default or const field: {}", property);
            }
        }

        const auto output_path = fs::absolute(".") / kb::path::from_str(task.get_destination_file());
        kb::io::write_file(output_path, output.dump(2) + "\n");
    }

    void generate_markdown(const config::MarkdownTask& task) {
        // TODO:
        LOG_INFO("{} -> NOT IMPLEMENTED", __func__);
    }
}

int wmain([[maybe_unused]] const int argc, [[maybe_unused]] const wchar_t* argv[]) { // NOLINT(*-use-internal-linkage)
    kb::logger::init_console_logger();
    spdlog::default_logger()->set_pattern("%-8l| %T.%e | %v");

    LOG_DEBUG("Running in: {}", kb::path::to_str(fs::current_path()));

    const auto config_path = fs::current_path() / "sync.json";
    if(!fs::exists(config_path)) {
        LOG_ERROR("Missing config file `sync.json` in current working directory.")
        exit(1);
    }
    const auto config_str = kb::io::read_file(config_path);
    config::options = nlohmann::json::parse(config_str).get<config::Config>();

    for(const auto& task : config::options.tasks) {
        std::visit(
            [&]<typename T>(const T& t) {
                LOG_DEBUG("Processing: {}", nlohmann::json(t).dump());

                if constexpr(std::is_same_v<std::decay_t<T>, config::TextTask>) {
                    generate_text(t);
                } else if constexpr(std::is_same_v<std::decay_t<T>, config::JsonTask>) {
                    generate_json(t);
                } else if constexpr(std::is_same_v<std::decay_t<T>, config::MarkdownTask>) {
                    generate_markdown(t);
                }
            }, task
        );
    }
}
