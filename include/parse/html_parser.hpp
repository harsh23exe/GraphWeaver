// html_parser.hpp - HTML Parsing Wrapper (Gumbo)
// LLM Documentation Scraper - C++ Implementation

#pragma once

#include <gumbo.h>
#include <string>
#include <vector>
#include <optional>

namespace docscraper::parse {

class HTMLElement;

class HTMLDocument {
public:
    explicit HTMLDocument(const std::string& html);
    ~HTMLDocument();

    HTMLDocument(const HTMLDocument&) = delete;
    HTMLDocument& operator=(const HTMLDocument&) = delete;

    HTMLElement root() const;
    std::vector<HTMLElement> select(const std::string& selector) const;
    std::optional<HTMLElement> select_first(const std::string& selector) const;

    const std::string& original_html() const { return original_html_; }

private:
    GumboOutput* output_ = nullptr;
    std::string original_html_;
};

class HTMLElement {
public:
    explicit HTMLElement(GumboNode* node);

    std::vector<HTMLElement> select(const std::string& selector) const;
    std::optional<HTMLElement> select_first(const std::string& selector) const;

    std::string tag_name() const;
    std::string attr(const std::string& name) const;
    bool has_class(const std::string& class_name) const;
    bool has_id(const std::string& id) const;

    std::string text() const;
    std::string html() const;

    GumboNode* node() const { return node_; }

private:
    GumboNode* node_;

    void collect_text(GumboNode* node, std::string& out) const;
};

} // namespace docscraper::parse
