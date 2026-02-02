// link_processor.hpp - Link extraction and rewriting
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include "parse/html_parser.hpp"
#include <string>
#include <vector>

namespace docscraper::process {

class LinkProcessor {
public:
    LinkProcessor() = default;

    std::vector<std::string> extract_links(const parse::HTMLDocument& doc) const;

    // Placeholder: in-place rewriting (no-op for now)
    void rewrite_links(
        parse::HTMLElement& /*content*/,
        const std::string& /*page_url*/,
        const std::string& /*site_output_dir*/,
        const std::string& /*allowed_domain*/
    ) const {}
};

} // namespace docscraper::process
