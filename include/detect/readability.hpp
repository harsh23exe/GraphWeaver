// readability.hpp - Simple Readability Fallback
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include "parse/html_parser.hpp"
#include <string>

namespace docscraper::detect {

struct ReadabilityResult {
    std::string title;
    std::string content;
    bool success = false;
};

class ReadabilityExtractor {
public:
    ReadabilityExtractor() = default;

    ReadabilityResult extract(const parse::HTMLDocument& doc);
};

} // namespace docscraper::detect
