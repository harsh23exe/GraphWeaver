#pragma once

#include "scrape_llm/types.hpp"
#include "scrape_llm/llm_client.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace scrapellm {

struct ValidationResult {
    bool valid = false;
    std::string error_message;
};

// Validate one record against JSON Schema (strict types). Simple subset of schema supported.
ValidationResult validate_record(const nlohmann::json& record, const nlohmann::json& json_schema);

// If invalid, run one repair attempt via LLM. Returns repaired record or nullopt if still invalid.
std::optional<nlohmann::json> repair_record(
    ILlmClient& client,
    const nlohmann::json& invalid_record,
    const nlohmann::json& json_schema,
    const std::string& validation_error
);

} // namespace scrapellm
