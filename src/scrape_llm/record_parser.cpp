#include "scrape_llm/record_parser.hpp"
#include <nlohmann/json.hpp>
#include <sstream>

namespace scrapellm {

static std::string build_page_content(const ExtractedContent& content) {
    std::ostringstream os;
    os << "Title: " << content.title << "\n";
    if (!content.meta_description.empty())
        os << "Description: " << content.meta_description << "\n\n";
    os << content.main_text << "\n\n";
    for (size_t i = 0; i < content.tables_tsv.size(); ++i) {
        os << "Table " << (i + 1) << ":\n" << content.tables_tsv[i] << "\n";
    }
    return os.str();
}

static std::string parse_records_prompt(const InferredSchema& schema, const ExtractedContent& content) {
    std::string content_str = build_page_content(content);
    std::string schema_str = schema.json_schema.dump(2);
    return "You are a structured data extractor. Extract records from the following page content so they conform to the given JSON Schema. "
           "Every record MUST include the field \"source_url\" with value exactly: " + content.url + "\n\n"
           "JSON Schema for one record:\n" + schema_str + "\n\n"
           "Extraction mode: " + schema.extraction_mode + "\n\n"
           "Page content:\n---\n" + content_str + "\n---\n\n"
           "If extraction_mode is \"single\", output a single JSON object. If \"list\", output a JSON array of objects. "
           "Output ONLY valid JSON. No markdown, no code fence, no explanation. Include source_url in every record.";
}

std::vector<nlohmann::json> parse_records(ILlmClient& client, const InferredSchema& schema, const ExtractedContent& content) {
    std::vector<nlohmann::json> out;
    client.set_json_mode(true);
    auto resp = client.chat(parse_records_prompt(schema, content), "");
    client.set_json_mode(false);
    if (!resp) return out;

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
        if (j.is_array()) {
            for (auto& item : j) {
                if (item.is_object()) {
                    if (!item.contains("source_url")) item["source_url"] = content.url;
                    out.push_back(std::move(item));
                }
            }
        } else if (j.is_object()) {
            if (!j.contains("source_url")) j["source_url"] = content.url;
            out.push_back(std::move(j));
        }
    } catch (...) {}
    return out;
}

} // namespace scrapellm
