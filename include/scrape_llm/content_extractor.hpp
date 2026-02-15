#pragma once

#include "scrape_llm/types.hpp"
#include <string>
#include <vector>

namespace docscraper::parse { class HTMLDocument; }

namespace scrapellm {

// Extract main text (strip script/style), tables as TSV-like, metadata.
ExtractedContent extract_content(const docscraper::parse::HTMLDocument& doc, const std::string& page_url);

// Lightweight digest for relevance: url, title, h1/h2, first N chars of main text.
PageDigest make_digest(const ExtractedContent& content, size_t max_preview_chars = 1500);

// Extract hrefs from <a> tags and resolve against base_url. Returns absolute URLs only.
std::vector<std::string> extract_links_absolute(
    const docscraper::parse::HTMLDocument& doc,
    const std::string& base_url
);

} // namespace scrapellm
