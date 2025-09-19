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
    namespace fs = std::filesystem;

    std::string jsonSchemaToExample(const std::string& json_schema_path) {
        // Parse the example for validation purposes
        auto json_schema_ifs = std::ifstream(json_schema_path);
        const auto json_schema = nlohmann::ordered_json::parse(json_schema_ifs);

        return json_schema["examples"][0].dump(2);
    }

    std::string jsonSchemaToConfigTable(const std::string& json_schema_path, bool advanced) {
        auto json_schema_ifs = std::ifstream(json_schema_path);
        const auto json_schema = nlohmann::ordered_json::parse(json_schema_ifs);

        std::ostringstream output;

        output << "| Option | Description | Type | Default | Valid values |\n";
        output << "|--------|-------------|------|---------|--------------|\n";

        for(const auto& [name, prop] : json_schema["properties"].items()) {
            // == here acts as an XNOR operator
            if(advanced == (name[0] != '$')) {
                continue;
            }

            std::string type = prop.at("type");
            type[0] = static_cast<char>(toupper(type[0]));

            const std::string default_value = prop.contains("x-default")
                                                  ? prop.at("x-default").get<std::string>()
                                                  : prop.contains("const") // NOLINT(*-avoid-nested-conditional-operator)
                                                  ? std::format("`{}`", prop.at("const").dump())
                                                  : std::format("`{}`", prop.at("default").dump());

            const auto valid_values = prop.at("x-valid-values").get<std::string>();

            output << std::format("| `{}` ", name);
            output << std::format("| {} ", prop.at("description").get<std::string>());
            output << std::format("| {} ", type);
            output << std::format("| {} ", inja::render(default_value, config::options.get_variables()));
            output << std::format("| {} ", inja::render(valid_values, config::options.get_variables()));

            output << "|\n";
        }

        return output.str();
    }

    // ReSharper disable once CppDFAUnreachableFunctionCall
    inja::Environment get_inja_env() {
        inja::Environment env;

        // inja uses ## by default for line statements, which leads to crashes in markdown templates.
        // Hence, we add a prefix to avoid such crashes.
        env.set_line_statement("inja::##");
        // env.set_lstrip_blocks(true);
        env.set_trim_blocks(true);

        // useful for filtering advanced properties in json schema
        env.add_callback(
            "startsWith", 2, [](const inja::Arguments& args) {
                const auto& value = args.at(0)->get<std::string>();
                const auto& prefix = args.at(1)->get<std::string>();
                return value.starts_with(prefix);
            }
        );

        // generating Markdown table within inja template is quite cumbersome, hence we do it programmatically instead.
        env.add_callback(
            "jsonSchemaToConfigTable", 2, [](const inja::Arguments& args) {
                const auto& json_schema_path = args.at(0)->get<std::string>();
                const auto& advanced = args.at(1)->get<bool>();
                return jsonSchemaToConfigTable(json_schema_path, advanced);
            }
        );

        // generating Markdown table within inja template is quite cumbersome, hence we do it programmatically instead.
        env.add_callback(
            "jsonSchemaToExample", 1, [](const inja::Arguments& args) {
                const auto& json_schema_path = args.at(0)->get<std::string>();
                return jsonSchemaToExample(json_schema_path);
            }
        );

        return env;
    }

    void generate_text(const config::TextTask& task) {
        auto env = get_inja_env();
        const auto rendered_str = env.render_file(task.get_template_file(), config::options.get_variables());

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
        std::ifstream schema_file_input(task.get_schema_file());
        const auto schema_json = nlohmann::ordered_json::parse(schema_file_input);

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

        const auto output_path = fs::absolute(kb::path::from_str(task.get_destination_file()));
        kb::io::write_file(output_path, output.dump(2) + "\n");
    }
}

int MAIN([[maybe_unused]] const int argc, [[maybe_unused]] const TCHAR* argv[]) { // NOLINT(*-use-internal-linkage)
    kb::logger::init_console_logger();
    spdlog::default_logger()->set_pattern("%-8l| %T.%e | %v");

    LOG_DEBUG("Running in: {}", kb::path::to_str(fs::current_path()));

    const std::string config_name = "sync.json";
    const auto config_path = fs::current_path() / config_name;
    if(!fs::exists(config_path)) {
        LOG_ERROR("Missing config file `{}` in current working directory.", config_name);
        exit(1);
    }
    const auto config_str = kb::io::read_file(config_path);
    config::options = nlohmann::json::parse(config_str).get<config::Config>();

    for(const auto& task : config::options.get_tasks()) {
        std::visit(
            [&]<typename T>(const T& t) {
                LOG_DEBUG("Processing: {}", nlohmann::json(t).dump());

                try {
                    if constexpr(std::is_same_v<std::decay_t<T>, config::TextTask>) {
                        generate_text(t);
                    } else if constexpr(std::is_same_v<std::decay_t<T>, config::JsonTask>) {
                        generate_json(t);
                    }
                } catch(const std::exception& e) {
                    LOG_ERROR("Uncaught exception: {}", e.what());
                    exit(1);
                }
            }, task
        );
    }

    kb::logger::shutdown();
}
