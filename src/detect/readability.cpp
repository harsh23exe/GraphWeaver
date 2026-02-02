// readability.cpp - Simple Readability Fallback
// LLM Documentation Scraper - C++ Implementation

#include "detect/readability.hpp"

namespace docscraper::detect {

ReadabilityResult ReadabilityExtractor::extract(const parse::HTMLDocument& doc) {
    ReadabilityResult result;

    // Prefer <article> or <main>, fallback to <body>
    auto article = doc.select_first("article");
    if (article.has_value()) {
        result.content = article->text();
        result.success = !result.content.empty();
        return result;
    }

    auto main = doc.select_first("main");
    if (main.has_value()) {
        result.content = main->text();
        result.success = !result.content.empty();
        return result;
    }

    auto body = doc.select_first("body");
    if (body.has_value()) {
        result.content = body->text();
        result.success = !result.content.empty();
        return result;
    }

    result.success = false;
    return result;
}

} // namespace docscraper::detect
