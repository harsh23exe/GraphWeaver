// link_processor.cpp - Link extraction
// LLM Documentation Scraper - C++ Implementation

#include "process/link_processor.hpp"

namespace docscraper::process {

std::vector<std::string> LinkProcessor::extract_links(const parse::HTMLDocument& doc) const {
    std::vector<std::string> links;
    auto anchors = doc.select("a");
    for (const auto& a : anchors) {
        auto href = a.attr("href");
        if (!href.empty()) {
            links.push_back(href);
        }
    }
    return links;
}

} // namespace docscraper::process
