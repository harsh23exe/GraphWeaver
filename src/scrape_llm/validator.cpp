#include "scrape_llm/validator.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace scrapellm {

static bool validate_type(const nlohmann::json& value, const std::string& type, std::string& err) {
    if (type == "string") { if (!value.is_string()) { err = "expected string"; return false; } return true; }
    if (type == "number") { if (!value.is_number()) { err = "expected number"; return false; } return true; }
    if (type == "integer") { if (!value.is_number_integer()) { err = "expected integer"; return false; } return true; }
    if (type == "boolean") { if (!value.is_boolean()) { err = "expected boolean"; return false; } return true; }
    if (type == "object") { if (!value.is_object()) { err = "expected object"; return false; } return true; }
    if (type == "array") { if (!value.is_array()) { err = "expected array"; return false; } return true; }
    if (type == "null") { if (!value.is_null()) { err = "expected null"; return false; } return true; }
    return true;
}

ValidationResult validate_record(const nlohmann::json& record, const nlohmann::json& json_schema) {
    ValidationResult out;
    if (!record.is_object()) {
        out.error_message = "record is not an object";
        return out;
    }
    if (!json_schema.is_object()) {
        out.valid = true;
        return out;
    }
    if (json_schema.contains("required") && json_schema["required"].is_array()) {
        for (const auto& key : json_schema["required"]) {
            std::string k = key.get<std::string>();
            if (!record.contains(k)) {
                out.error_message = "missing required field: " + k;
                return out;
            }
        }
    }
    if (json_schema.contains("properties") && json_schema["properties"].is_object()) {
        for (auto it = json_schema["properties"].begin(); it != json_schema["properties"].end(); ++it) {
            std::string key = it.key();
            if (!record.contains(key)) continue;
            const auto& prop_schema = it.value();
            if (prop_schema.is_object() && prop_schema.contains("type")) {
                std::string type = prop_schema["type"].get<std::string>();
                if (!validate_type(record[key], type, out.error_message)) {
                    out.error_message = "field '" + key + "': " + out.error_message;
                    return out;
                }
            }
        }
    }
    out.valid = true;
    return out;
}

static std::string repair_prompt(const nlohmann::json& schema, const nlohmann::json& invalid, const std::string& err) {
    return "You are a JSON repair assistant. The following record failed JSON Schema validation. "
           "Output the corrected record as a single JSON object only. Do not output anything else (no markdown, no explanation).\n\n"
           "JSON Schema:\n" + schema.dump(2) + "\n\n"
           "Invalid record:\n" + invalid.dump(2) + "\n\n"
           "Validation error:\n" + err + "\n\n"
           "Output ONLY the corrected JSON object.";
}

std::optional<nlohmann::json> repair_record(ILlmClient& client, const nlohmann::json& invalid_record,
                                            const nlohmann::json& json_schema, const std::string& validation_error) {
    auto resp = client.chat(repair_prompt(json_schema, invalid_record, validation_error), "");
    if (!resp) return std::nullopt;
    std::string raw = *resp;
    if (raw.size() >= 3 && raw.substr(0, 3) == "```") {
        size_t end = raw.find('\n');
        if (end != std::string::npos) raw = raw.substr(end + 1);
        size_t close = raw.find("```");
        if (close != std::string::npos) raw = raw.substr(0, close);
    }
    try {
        auto j = nlohmann::json::parse(raw);
        if (j.is_object()) return j;
    } catch (...) {}
    return std::nullopt;
}

} // namespace scrapellm
