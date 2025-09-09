#include <filesystem>
#include <variant>

#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

#include <koalabox/io.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/path.hpp>

namespace kb = koalabox;

namespace config {
    enum class GeneratorType {
        Text,
        Json,
        Markdown
    };

// @formatter:off
NLOHMANN_JSON_SERIALIZE_ENUM(GeneratorType, { // NOLINT(*-use-internal-linkage)
    { GeneratorType::Text,     "text" },
    { GeneratorType::Json,     "json" },
    { GeneratorType::Markdown, "markdown" },
}); // @formatter:on

    struct TextGenerator {
        GeneratorType type = GeneratorType::Text;
        std::string template_file;
        std::string destination_dir;
        std::string file_name;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
            TextGenerator,
            type, template_file, destination_dir, file_name
        );
    };

    struct JsonGenerator {
        GeneratorType type = GeneratorType::Json;
        std::string schema_file;
        std::string destination_file;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
            JsonGenerator,
            type, schema_file, destination_file
        );
    };

    struct MarkdownGenerator {
        GeneratorType type = GeneratorType::Markdown;
        std::string file_path;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
            MarkdownGenerator,
            type, file_path
        );
    };

    using Generator = std::variant<TextGenerator, JsonGenerator, MarkdownGenerator>;

    struct Config {
        std::string templates_dir;
        std::map<std::string, std::string> variables;
        std::vector<Generator> generators;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
            Config,
            templates_dir, variables, generators
        );
    };
}

template<>
struct nlohmann::adl_serializer<config::Generator> {
    static void from_json(const json& j, config::Generator& generator) {
        if(!j.contains("type")) {
            return;
        }

        const config::GeneratorType type = j["type"];

        if(type == config::GeneratorType::Text) {
            generator = config::TextGenerator(j);
        } else if(type == config::GeneratorType::Json) {
            generator = config::JsonGenerator(j);
        } else if(type == config::GeneratorType::Markdown) {
            generator = config::MarkdownGenerator(j);
        }
    }

    static void to_json(json& j, const config::Generator& generator) {
        if(std::holds_alternative<config::TextGenerator>(generator)) {
            j = std::get<config::TextGenerator>(generator);
        } else if(std::holds_alternative<config::JsonGenerator>(generator)) {
            j = std::get<config::JsonGenerator>(generator);
        } else {
            j = std::get<config::MarkdownGenerator>(generator);
        }
    }
};

namespace {
    void process_text(const config::Config& config, config::TextGenerator generator) {
        if(generator.template_file.empty()) {
            LOG_ERROR("template_file cannot be empty:\n{}", nlohmann::json(generator).dump(2));
            exit(1);
        }

        const auto template_path = kb::path::from_str(config.templates_dir) / generator.template_file;
        if(!fs::exists(template_path)) {
            LOG_ERROR("template_file not found:\n{}", kb::path::to_str(fs::absolute(template_path)));
            exit(1);
        }

        const auto template_str = kb::io::read_file(template_path);

        std::string rendered_str;
        try {
            rendered_str = inja::render(template_str, config.variables);
        } catch(const std::exception& e) {
            LOG_ERROR("Error parsing template {}: {}", generator.template_file, e.what());
            exit(1);
        }

        const auto output_dir = fs::absolute(
            kb::path::from_str(generator.destination_dir.empty() ? "." : generator.destination_dir)
        );
        const auto output_file = generator.file_name.empty()
                                          ? kb::path::to_str(kb::path::from_str(generator.template_file).filename())
                                          : generator.file_name;
        const auto output_path = output_dir / output_file;
        kb::io::write_file(output_path, rendered_str);
    }

    void process_json(const config::Config&, config::JsonGenerator generator) {
        // TODO
    }

    void process_markdown(const config::Config&, config::MarkdownGenerator generator) {
        // TODO
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
    const auto config = nlohmann::json::parse(config_str).get<config::Config>();

    for(const auto& generator : config.generators) {
        std::visit(
            [&]<typename G>(const G& g) {
                LOG_DEBUG("Processing: {}", nlohmann::json(generator).dump());
                if constexpr(std::is_same_v<std::decay_t<G>, config::TextGenerator>) {
                    process_text(config, g);
                } else if constexpr(std::is_same_v<std::decay_t<G>, config::JsonGenerator>) {
                    process_json(config, g);
                } else if constexpr(std::is_same_v<std::decay_t<G>, config::MarkdownGenerator>) {
                    process_markdown(config, g);
                }
            }, generator
        );
    }
}
