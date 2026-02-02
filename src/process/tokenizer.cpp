// tokenizer.cpp - Token counting utility
// LLM Documentation Scraper - C++ Implementation

#include "process/tokenizer.hpp"

namespace docscraper::process {

int Tokenizer::count_tokens(const std::string& text) const {
    // Simple heuristic: ~4 characters per token
    if (text.empty()) return 0;
    return static_cast<int>(text.size() / 4) + 1;
}

int count_tokens(const std::string& text) {
    return Tokenizer().count_tokens(text);
}

} // namespace docscraper::process
