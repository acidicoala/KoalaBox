#include <koalabox/json.hpp>
#include <koalabox/io.hpp>
#include <utility>

Json::Json(nlohmann::json instance) : instance(std::move(instance)) {}

Json::Json(const String& json_string) : Json(nlohmann::json::parse(json_string)) {}

Json::Json(const Path& json_path) : Json(koalabox::io::read_file(json_path)) {}

Json::Json() : Json(nlohmann::json()) {}

Json Json::operator[](const String& key) const {
    // nlohmann json will throw exception with .at method, as opposed to its subscript operator overload
    return Json(instance.at(key));
}

bool Json::is_null() const {
    return instance.is_null();
}

String Json::to_string() const {
    return instance.dump(2);
}
