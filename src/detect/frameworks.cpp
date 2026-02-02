// frameworks.cpp - Framework Signatures
// LLM Documentation Scraper - C++ Implementation

#include "detect/frameworks.hpp"
#include <algorithm>

namespace docscraper::detect {

static bool html_contains(const parse::HTMLDocument& doc, const std::string& needle) {
    std::string hay = doc.original_html();
    std::string n = needle;
    std::transform(hay.begin(), hay.end(), hay.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    std::transform(n.begin(), n.end(), n.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return hay.find(n) != std::string::npos;
}

std::vector<FrameworkSignature> get_framework_signatures() {
    return {
        {
            Framework::Docusaurus,
            "article, main.mainContainer, div.docMainContainer",
            [](const parse::HTMLDocument& doc) {
                auto d1 = doc.select(".docusaurus");
                return !d1.empty() || html_contains(doc, "docusaurus");
            }
        },
        {
            Framework::Sphinx,
            "div.body, div[role='main'], div.document, article",
            [](const parse::HTMLDocument& doc) {
                auto s1 = doc.select(".document");
                return !s1.empty() || html_contains(doc, "sphinx");
            }
        },
        {
            Framework::MkDocs,
            "div.md-content, main, article",
            [](const parse::HTMLDocument& doc) {
                auto m2 = doc.select(".md-content");
                return !m2.empty() || html_contains(doc, "mkdocs");
            }
        },
        {
            Framework::GitBook,
            "div.book, div.book-body, article",
            [](const parse::HTMLDocument& doc) {
                auto g1 = doc.select(".book");
                return !g1.empty() || html_contains(doc, "gitbook");
            }
        },
        {
            Framework::ReadTheDocs,
            "div.rst-content, div[role='main'], article",
            [](const parse::HTMLDocument& doc) {
                auto r1 = doc.select(".rst-content");
                return !r1.empty() || html_contains(doc, "read the docs");
            }
        }
    };
}

} // namespace docscraper::detect
