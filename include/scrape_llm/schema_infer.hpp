#pragma once

#include "scrape_llm/types.hpp"
#include "scrape_llm/llm_client.hpp"
#include <string>

namespace scrapellm {

// Infer schema from natural-language description. Uses LLM; on failure returns fallback and sets warning.
InferredSchema schema_infer(ILlmClient& client, const std::string& user_schema, std::string& out_warning);

// Fallback schema when LLM fails: { "source_url": "string", "content": "string" }.
InferredSchema fallback_schema();

} // namespace scrapellm
