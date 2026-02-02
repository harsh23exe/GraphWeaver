// detector.cpp - Framework Detector Implementation
// LLM Documentation Scraper - C++ Implementation

#include "detect/detector.hpp"
#include "detect/frameworks.hpp"
#include "parse/normalizer.hpp"

namespace docscraper::detect {

bool is_auto_selector(const std::string& selector) {
    return selector == "auto";
}

std::string framework_to_string(Framework fw) {
    switch (fw) {
        case Framework::Docusaurus: return "Docusaurus";
        case Framework::MkDocs: return "MkDocs";
        case Framework::Sphinx: return "Sphinx";
        case Framework::GitBook: return "GitBook";
        case Framework::ReadTheDocs: return "ReadTheDocs";
        default: return "Unknown";
    }
}

DetectionResult ContentDetector::detect(const parse::HTMLDocument& doc, const std::string& url) {
    // Cache by domain
    std::string domain = parse::URLNormalizer::extract_domain(url);
    if (!domain.empty()) {
        auto it = cache_.find(domain);
        if (it != cache_.end()) {
            return it->second;
        }
    }

    auto result = detect_framework(doc);
    if (result.framework == Framework::Unknown) {
        result.selector = "article, main, body";
        result.fallback = true;
    }

    if (!domain.empty()) {
        cache_[domain] = result;
    }
    return result;
}

DetectionResult ContentDetector::detect_framework(const parse::HTMLDocument& doc) {
    for (const auto& sig : get_framework_signatures()) {
        if (sig.matcher(doc)) {
            DetectionResult res;
            res.framework = sig.type;
            res.selector = sig.selector;
            res.fallback = false;
            return res;
        }
    }

    return DetectionResult{};
}

} // namespace docscraper::detect
