#pragma once

#include "scrape_llm/types.hpp"
#include "scrape_llm/llm_client.hpp"
#include <nlohmann/json.hpp>
#include <vector>

namespace scrapellm {

// Parse one page's content into records conforming to schema. Injects source_url.
std::vector<nlohmann::json> parse_records(
    ILlmClient& client,
    const InferredSchema& schema,
    const ExtractedContent& content
);

} // namespace scrapellm
