#pragma once

#include "scrape_llm/types.hpp"
#include "scrape_llm/llm_client.hpp"
#include <string>
#include <vector>

namespace scrapellm {

struct RelevanceDecision {
    bool keep = false;
    std::string reason;
};

// LLM decides KEEP or SKIP for a page digest.
RelevanceDecision relevance_decide(
    ILlmClient& client,
    const std::string& user_schema,
    const PageDigest& digest
);

// From crawled pages with digests, select top keep_n by relevance (LLM score/decision).
std::vector<PageDigest> select_pages_to_parse(
    ILlmClient& client,
    const std::string& user_schema,
    std::vector<PageDigest> digests,
    int keep_n
);

} // namespace scrapellm
