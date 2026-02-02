// tokenizer.hpp - Token counting utility
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include <string>

namespace docscraper::process {

class Tokenizer {
public:
    Tokenizer() = default;
    int count_tokens(const std::string& text) const;
};

int count_tokens(const std::string& text);

} // namespace docscraper::process
