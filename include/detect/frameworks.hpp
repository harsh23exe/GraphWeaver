// frameworks.hpp - Framework Signatures
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include "detect/detector.hpp"
#include <functional>
#include <vector>

namespace docscraper::detect {

struct FrameworkSignature {
    Framework type;
    std::string selector;
    std::function<bool(const parse::HTMLDocument&)> matcher;
};

std::vector<FrameworkSignature> get_framework_signatures();

} // namespace docscraper::detect
