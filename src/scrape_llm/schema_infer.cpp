#include "scrape_llm/schema_infer.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace scrapellm {

InferredSchema fallback_schema() {
    InferredSchema s;
    s.json_schema = {
        {"type", "object"},
        {"properties", {
            {"source_url", {{"type", "string"}}},
            {"content", {{"type", "string"}}}
        }},
        {"required", nlohmann::json::array({"source_url", "content"})}
    };
    s.extraction_mode = "list";
    s.hints = nlohmann::json::object();
    return s;
}

static std::string schema_infer_prompt(const std::string& user_schema) {
    return "You are a schema inference assistant. Given a natural-language description of the data to extract from web pages, output a strict JSON object (and nothing else) with this exact structure:\n"
           "{\n"
           "  \"json_schema\": { ... },\n"
           "  \"extraction_mode\": \"single\" or \"list\",\n"
           "  \"hints\": {\n"
           "    \"item_selector_hint\": \"optional\",\n"
           "    \"key_fields\": [],\n"
           "    \"dedupe_key\": \"field_name\" or []\n"
           "  }\n"
           "}\n\n"
           "User description of desired data:\n" + user_schema + "\n\n"
           "Output ONLY valid JSON. No markdown, no code fence, no explanation.";
}

InferredSchema schema_infer(ILlmClient& client, const std::string& user_schema, std::string& out_warning) {
    out_warning.clear();
    client.set_json_mode(true);
    auto resp = client.chat(schema_infer_prompt(user_schema), "");
    client.set_json_mode(false);

    if (!resp) {
        out_warning = "Schema inference LLM call failed; using fallback schema.";
        spdlog::warn("{}", out_warning);
        return fallback_schema();
    }

    std::string raw = *resp;
    while (!raw.empty() && (raw.front() == '\n' || raw.front() == ' ')) raw.erase(0, 1);
    if (raw.size() >= 3 && raw.substr(0, 3) == "```") {
        size_t end = raw.find('\n');
        if (end != std::string::npos) raw = raw.substr(end + 1);
        size_t close = raw.find("```");
        if (close != std::string::npos) raw = raw.substr(0, close);
    }

    try {
        auto j = nlohmann::json::parse(raw);
        if (!j.contains("json_schema") || !j.contains("extraction_mode")) {
            out_warning = "Schema response missing json_schema or extraction_mode; using fallback.";
            spdlog::warn("{}", out_warning);
            return fallback_schema();
        }
        InferredSchema s;
        s.json_schema = j["json_schema"];
        s.extraction_mode = j["extraction_mode"].get<std::string>();
        if (s.extraction_mode != "single" && s.extraction_mode != "list")
            s.extraction_mode = "list";
        s.hints = j.contains("hints") && j["hints"].is_object() ? j["hints"] : nlohmann::json::object();
        return s;
    } catch (const std::exception& e) {
        out_warning = "Schema inference parse error: ";
        out_warning += e.what();
        out_warning += "; using fallback schema.";
        spdlog::warn("{}", out_warning);
        return fallback_schema();
    }
}

} // namespace scrapellm
