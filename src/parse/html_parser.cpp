// html_parser.cpp - HTML Parsing Wrapper (Gumbo)
// LLM Documentation Scraper - C++ Implementation

#include "parse/html_parser.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace docscraper::parse {

static std::string to_lower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

static void find_matching_nodes(GumboNode* node,
                                const std::string& selector,
                                std::vector<GumboNode*>& results);

// Very simple CSS selector support:
// - tag (e.g., "main")
// - .class (e.g., ".content")
// - #id (e.g., "#root")
// - tag.class (e.g., "div.content")
// - tag#id (e.g., "section#docs")
static bool matches_selector(GumboNode* node, const std::string& selector) {
    if (!node || node->type != GUMBO_NODE_ELEMENT) return false;

    GumboTag tag = node->v.element.tag;
    std::string tag_name = to_lower(gumbo_normalized_tagname(tag));

    std::string sel = selector;
    sel.erase(std::remove_if(sel.begin(), sel.end(), ::isspace), sel.end());
    if (sel.empty()) return false;

    std::string sel_tag;
    std::string sel_class;
    std::string sel_id;

    // Parse selector
    size_t dot_pos = sel.find('.');
    size_t hash_pos = sel.find('#');
    size_t split_pos = std::min(dot_pos == std::string::npos ? sel.size() : dot_pos,
                                hash_pos == std::string::npos ? sel.size() : hash_pos);

    if (split_pos > 0 && split_pos != std::string::npos) {
        sel_tag = to_lower(sel.substr(0, split_pos));
    }

    if (dot_pos != std::string::npos) {
        size_t end = (hash_pos != std::string::npos && hash_pos > dot_pos)
                         ? hash_pos
                         : sel.size();
        sel_class = sel.substr(dot_pos + 1, end - dot_pos - 1);
    }

    if (hash_pos != std::string::npos) {
        sel_id = sel.substr(hash_pos + 1);
    }

    if (!sel_tag.empty() && sel_tag != tag_name) {
        return false;
    }

    GumboAttribute* id_attr = gumbo_get_attribute(&node->v.element.attributes, "id");
    if (!sel_id.empty()) {
        if (!id_attr || sel_id != id_attr->value) return false;
    }

    if (!sel_class.empty()) {
        GumboAttribute* class_attr = gumbo_get_attribute(&node->v.element.attributes, "class");
        if (!class_attr) return false;
        std::string classes = class_attr->value;
        std::istringstream iss(classes);
        std::string cls;
        bool found = false;
        while (iss >> cls) {
            if (cls == sel_class) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }

    return true;
}

static void find_matching_nodes(GumboNode* node,
                                const std::string& selector,
                                std::vector<GumboNode*>& results) {
    if (!node) return;

    if (matches_selector(node, selector)) {
        results.push_back(node);
    }

    if (node->type == GUMBO_NODE_ELEMENT) {
        GumboVector* children = &node->v.element.children;
        for (size_t i = 0; i < children->length; ++i) {
            find_matching_nodes(static_cast<GumboNode*>(children->data[i]), selector, results);
        }
    }
}

HTMLDocument::HTMLDocument(const std::string& html)
    : original_html_(html) {
    output_ = gumbo_parse(html.c_str());
}

HTMLDocument::~HTMLDocument() {
    if (output_) {
        gumbo_destroy_output(&kGumboDefaultOptions, output_);
    }
}

HTMLElement HTMLDocument::root() const {
    return HTMLElement(output_->root);
}

std::vector<HTMLElement> HTMLDocument::select(const std::string& selector) const {
    std::vector<HTMLElement> elements;
    if (!output_ || !output_->root) return elements;

    // Support comma-separated selectors
    std::istringstream iss(selector);
    std::string sel;
    while (std::getline(iss, sel, ',')) {
        std::vector<GumboNode*> nodes;
        find_matching_nodes(output_->root, sel, nodes);
        for (auto* node : nodes) {
            elements.emplace_back(node);
        }
    }

    return elements;
}

std::optional<HTMLElement> HTMLDocument::select_first(const std::string& selector) const {
    auto results = select(selector);
    if (results.empty()) return std::nullopt;
    return results.front();
}

static void collect_main_text(GumboNode* node, std::string& out) {
    if (!node) return;
    if (node->type == GUMBO_NODE_TEXT) {
        out += node->v.text.text;
        out += " ";
        return;
    }
    if (node->type != GUMBO_NODE_ELEMENT) return;
    GumboTag tag = node->v.element.tag;
    if (tag == GUMBO_TAG_SCRIPT || tag == GUMBO_TAG_STYLE || tag == GUMBO_TAG_NOSCRIPT)
        return;
    GumboVector* children = &node->v.element.children;
    for (size_t i = 0; i < children->length; ++i) {
        collect_main_text(static_cast<GumboNode*>(children->data[i]), out);
    }
}

std::string HTMLDocument::main_text() const {
    std::string out;
    if (!output_ || !output_->root) return out;
    auto body_opt = select_first("body");
    GumboNode* start = body_opt ? body_opt->node() : output_->root;
    if (start)
        collect_main_text(start, out);
    while (!out.empty() && std::isspace(static_cast<unsigned char>(out.back()))) out.pop_back();
    return out;
}

HTMLElement::HTMLElement(GumboNode* node)
    : node_(node) {}

std::vector<HTMLElement> HTMLElement::select(const std::string& selector) const {
    std::vector<HTMLElement> elements;
    if (!node_) return elements;

    std::istringstream iss(selector);
    std::string sel;
    while (std::getline(iss, sel, ',')) {
        std::vector<GumboNode*> nodes;
        find_matching_nodes(node_, sel, nodes);
        for (auto* n : nodes) {
            elements.emplace_back(n);
        }
    }
    return elements;
}

std::optional<HTMLElement> HTMLElement::select_first(const std::string& selector) const {
    auto results = select(selector);
    if (results.empty()) return std::nullopt;
    return results.front();
}

std::string HTMLElement::tag_name() const {
    if (!node_ || node_->type != GUMBO_NODE_ELEMENT) return "";
    return gumbo_normalized_tagname(node_->v.element.tag);
}

std::string HTMLElement::attr(const std::string& name) const {
    if (!node_ || node_->type != GUMBO_NODE_ELEMENT) return "";
    GumboAttribute* attr = gumbo_get_attribute(&node_->v.element.attributes, name.c_str());
    return attr ? attr->value : "";
}

bool HTMLElement::has_class(const std::string& class_name) const {
    if (!node_ || node_->type != GUMBO_NODE_ELEMENT) return false;
    GumboAttribute* attr = gumbo_get_attribute(&node_->v.element.attributes, "class");
    if (!attr) return false;
    std::istringstream iss(attr->value);
    std::string cls;
    while (iss >> cls) {
        if (cls == class_name) return true;
    }
    return false;
}

bool HTMLElement::has_id(const std::string& id) const {
    if (!node_ || node_->type != GUMBO_NODE_ELEMENT) return false;
    GumboAttribute* attr = gumbo_get_attribute(&node_->v.element.attributes, "id");
    return attr && id == attr->value;
}

void HTMLElement::collect_text(GumboNode* node, std::string& out) const {
    if (!node) return;
    if (node->type == GUMBO_NODE_TEXT) {
        out += node->v.text.text;
        out += " ";
    } else if (node->type == GUMBO_NODE_ELEMENT) {
        GumboVector* children = &node->v.element.children;
        for (size_t i = 0; i < children->length; ++i) {
            collect_text(static_cast<GumboNode*>(children->data[i]), out);
        }
    }
}

std::string HTMLElement::text() const {
    std::string out;
    collect_text(node_, out);
    // Trim
    while (!out.empty() && std::isspace(static_cast<unsigned char>(out.back()))) out.pop_back();
    return out;
}

std::string HTMLElement::html() const {
    // Simple fallback: return text content when full HTML serialization is unavailable
    return text();
}

} // namespace docscraper::parse
