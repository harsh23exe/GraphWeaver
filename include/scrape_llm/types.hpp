#pragma once

#include <nlohmann/json.hpp>
#include <chrono>
#include <string>
#include <vector>

namespace scrapellm {

using json = nlohmann::json;

struct InferredSchema {
    json json_schema;
    std::string extraction_mode;  // "single" | "list"
    json hints;                  // item_selector_hint, key_fields, dedupe_key, etc.
};

struct PageDigest {
    std::string url;
    std::string title;
    std::vector<std::string> headings;
    std::string text_preview;
};

struct ExtractedContent {
    std::string url;
    std::string main_text;
    std::vector<std::string> tables_tsv;  // each table as TSV-like string
    std::string title;
    std::string meta_description;
    std::vector<std::string> headings;    // h1, h2 (and optionally h3) in order
};

struct RunReport {
    int pages_crawled = 0;
    int pages_kept = 0;
    int records_emitted = 0;
    int validation_failures = 0;
    int repair_attempts = 0;
    int repair_successes = 0;
    int64_t tokens_estimate = 0;
    std::chrono::milliseconds crawl_ms{0};
    std::chrono::milliseconds llm_ms{0};
    std::vector<std::string> errors;
    std::vector<std::string> pages_visited;
};

} // namespace scrapellm
