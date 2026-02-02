// markdown_converter.hpp - HTML to Markdown conversion
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include <string>

namespace docscraper::process {

class MarkdownConverter {
public:
    MarkdownConverter() = default;

    // Convert HTML string to Markdown
    std::string convert(const std::string& html);

private:
    std::string strip_tags(const std::string& html);
};

} // namespace docscraper::process
