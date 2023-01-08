#pragma once

#include <koalabox/core.hpp>
#include <nlohmann/json.hpp>

/**
 * A wrapper class for handling Json data types. It's purpose is to abstract
 * actual json parser library from the rest of the code base. At the moment
 * nlohmann::json does all the heavy lifting. All constructors will throw
 * exceptions if they encounter errors, hence instantiations of this class
 * must always happen in try-catch blocks.
 */
class Json {
private:
    nlohmann::json instance;

    explicit Json(nlohmann::json instance);

    friend void to_json(nlohmann::json& nlohmann_json_j, const Json& nlohmann_json_t) {
        nlohmann_json_j = nlohmann_json_t.instance;
    }

    friend void from_json(const nlohmann::json& nlohmann_json_j, Json& nlohmann_json_t) {
        nlohmann_json_t.instance = nlohmann_json_j;
    }

public:
    explicit Json();
    explicit Json(const String& json_string);
    explicit Json(const Path& json_path);

    template<class T>
    explicit Json(const T& arbitrary): Json(nlohmann::json(arbitrary)) {}

    template<class T>
    T get() const {
        return instance.get<T>();
    }

    Json operator[](const String& key) const;

    [[nodiscard]] bool is_null() const;

    [[nodiscard]] String to_string() const;

};
