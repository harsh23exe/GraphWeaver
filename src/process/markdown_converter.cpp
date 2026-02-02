// markdown_converter.cpp - HTML to Markdown conversion
// LLM Documentation Scraper - C++ Implementation

#include "process/markdown_converter.hpp"
#include <regex>
#include <sstream>

namespace docscraper::process {

std::string MarkdownConverter::strip_tags(const std::string& html) {
    static const std::regex tags("<[^>]*>");
    return std::regex_replace(html, tags, "");
}

std::string MarkdownConverter::convert(const std::string& html) {
    // Minimal conversion: handle headings and paragraphs
    std::string md = html;

    md = std::regex_replace(md, std::regex("<h1[^>]*>(.*?)</h1>", std::regex::icase), "# $1\n\n");
    md = std::regex_replace(md, std::regex("<h2[^>]*>(.*?)</h2>", std::regex::icase), "## $1\n\n");
    md = std::regex_replace(md, std::regex("<h3[^>]*>(.*?)</h3>", std::regex::icase), "### $1\n\n");
    md = std::regex_replace(md, std::regex("<p[^>]*>(.*?)</p>", std::regex::icase), "$1\n\n");
    md = std::regex_replace(md, std::regex("<strong[^>]*>(.*?)</strong>", std::regex::icase), "**$1**");
    md = std::regex_replace(md, std::regex("<em[^>]*>(.*?)</em>", std::regex::icase), "*$1*");
    md = std::regex_replace(md, std::regex("<code[^>]*>(.*?)</code>", std::regex::icase), "`$1`");

    // Strip remaining tags
    md = strip_tags(md);

    // Normalize blank lines
    md = std::regex_replace(md, std::regex("\n{3,}"), "\n\n");

    return md;
}

} // namespace docscraper::process
