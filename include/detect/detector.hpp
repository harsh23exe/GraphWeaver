// detector.hpp - Framework Detection
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include "parse/html_parser.hpp"
#include <string>
#include <map>

namespace docscraper::detect {

enum class Framework {
    Unknown,
    Docusaurus,
    MkDocs,
    Sphinx,
    GitBook,
    ReadTheDocs
};

struct DetectionResult {
    Framework framework = Framework::Unknown;
    std::string selector;
    bool fallback = false;
};

class ContentDetector {
public:
    ContentDetector() = default;

    DetectionResult detect(
        const parse::HTMLDocument& doc,
        const std::string& url
    );

private:
    std::map<std::string, DetectionResult> cache_;
    DetectionResult detect_framework(const parse::HTMLDocument& doc);
};

bool is_auto_selector(const std::string& selector);
std::string framework_to_string(Framework fw);

} // namespace docscraper::detect
